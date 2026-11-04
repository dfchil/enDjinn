#include <enDjinn/enj_defs.h>
#include <enDjinn/enj_vmu.h>

#include <stdlib.h>
#include <string.h>
#ifdef __DREAMCAST__
#include <malloc.h>
#endif

/*
 * On the Dreamcast the save is a VMS file: a header with the descriptions and
 * icon, a CRC, then the data. It is read and written whole with O_META, so a
 * file whose CRC is wrong is rejected instead of handed back as data, and the
 * write's result is fs_close's (fs_vmu writes to the VMU when it closes).
 *
 * The host backends keep /vmu/ paths as plain files, so there the data is
 * written as it is, at once, with no thread.
 */

#define ENJ_VMU_PATH_MAX 32
#define ENJ_VMU_RETRY_MS 5000      /* no VMU, or a write failed */
#define ENJ_VMU_LATE_RETRY_MS 30000 /* a VMU plugged in late holds a save */

static char vmu_path[ENJ_VMU_PATH_MAX]; /* where the save was read from, or will go */
static int vmu_checked;                 /* that VMU was looked at for a save */

#ifdef __DREAMCAST__

/** The save's path on the first VMU, or on the first that holds it if mustExist. */
static int vmu_find(const enj_vmu_save_t *save, char *out, int mustExist) {
  for (int i = 0; i < MAPLE_PORT_COUNT * 2; i++) {
    maple_device_t *dev = maple_enum_type(i, MAPLE_FUNC_MEMCARD);
    if (dev == NULL || !dev->valid) {
      continue;
    }
    snprintf(out, ENJ_VMU_PATH_MAX, "/vmu/%c%d/%s", 'a' + dev->port,
             dev->unit, save->name);
    if (!mustExist) {
      return 1;
    }
    file_t fd = fs_open(out, O_RDONLY | O_META);
    if (fd != FILEHND_INVALID) {
      fs_close(fd);
      return 1;
    }
  }
  return 0;
}

enj_vmu_load_e enj_vmu_save_load(const enj_vmu_save_t *save) {
  if (!vmu_find(save, vmu_path, 1)) {
    if (vmu_find(save, vmu_path, 0)) {
      vmu_checked = 1; /* a VMU with no save yet: the first write goes there */
      return ENJ_VMU_NO_SAVE;
    }
    vmu_path[0] = '\0';
    return ENJ_VMU_NO_DEVICE;
  }
  file_t fd = fs_open(vmu_path, O_RDONLY | O_META);
  if (fd == FILEHND_INVALID) {
    return ENJ_VMU_UNREADABLE;
  }
  size_t bytes = fs_total(fd);
  uint8_t *file = malloc(bytes);
  ssize_t got = file != NULL ? fs_read(fd, file, bytes) : -1;
  fs_close(fd);

  enj_vmu_load_e result = ENJ_VMU_UNREADABLE;
  vmu_pkg_t pkg;
  if (got == (ssize_t)bytes && vmu_pkg_parse(file, bytes, &pkg) == 0 &&
      (size_t)pkg.data_len == save->size) {
    memcpy(save->data, pkg.data, save->size);
    vmu_checked = 1;
    result = ENJ_VMU_LOADED;
  }
  free(file);
  return result;
}

static int vmu_write(const enj_vmu_save_t *save, const void *data) {
  vmu_pkg_t pkg;
  memset(&pkg, 0, sizeof(pkg));
  strncpy(pkg.desc_short, save->desc_short ? save->desc_short : "",
          sizeof(pkg.desc_short) - 1);
  strncpy(pkg.desc_long, save->desc_long ? save->desc_long : "",
          sizeof(pkg.desc_long) - 1);
  strncpy(pkg.app_id, save->app_id ? save->app_id : "",
          sizeof(pkg.app_id) - 1);
  if (save->icon_palette != NULL && save->icon_frames != NULL &&
      save->icon_frame_count > 0) {
    pkg.icon_cnt = save->icon_frame_count > 3 ? 3 : save->icon_frame_count;
    pkg.icon_anim_speed = save->icon_anim_speed;
    memcpy(pkg.icon_pal, save->icon_palette, sizeof(pkg.icon_pal));
    pkg.icon_data = (uint8_t *)save->icon_frames;
  }
  pkg.data_len = (int)save->size;
  pkg.data = data;

  uint8_t *file = NULL;
  int bytes = 0;
  if (vmu_pkg_build(&pkg, &file, &bytes) < 0 || file == NULL) {
    return -1;
  }
  /* O_TRUNC: fs_vmu replaces the old file only when the new one is written. */
  file_t fd = fs_open(vmu_path, O_WRONLY | O_TRUNC | O_META);
  int ok = 0;
  if (fd != FILEHND_INVALID) {
    ssize_t written = fs_write(fd, file, bytes);
    ok = fs_close(fd) == 0 && written == bytes;
  }
  free(file);
  return ok ? 0 : -1;
}

/** Where the save goes, or 0 with *retry_ms set when it should not be written now. */
static int vmu_target(const enj_vmu_save_t *save, unsigned *retry_ms) {
  if (vmu_path[0] == '\0' && !vmu_find(save, vmu_path, 0)) {
    *retry_ms = ENJ_VMU_RETRY_MS; /* no VMU */
    return 0;
  }
  if (!vmu_checked) {
    /* Plugged in after the load: a save on it was never read; leave it be. */
    char existing[ENJ_VMU_PATH_MAX];
    if (vmu_find(save, existing, 1)) {
      printf("enj_vmu: %s holds a save that was not loaded; not writing over it\n",
             existing);
      *retry_ms = ENJ_VMU_LATE_RETRY_MS;
      return 0;
    }
    vmu_checked = 1;
  }
  return 1;
}

static unsigned vmu_write_now(const enj_vmu_save_t *save, const void *data) {
  unsigned retry_ms = 0;
  if (!vmu_target(save, &retry_ms)) {
    return retry_ms;
  }
  if (vmu_write(save, data) != 0) {
    /* The VMU may have moved to another port. */
    if (!vmu_find(save, vmu_path, 0) || vmu_write(save, data) != 0) {
      printf("enj_vmu: writing %s failed; trying again\n", vmu_path);
      return ENJ_VMU_RETRY_MS;
    }
  }
  return 0;
}

int enj_vmu_save_write(const enj_vmu_save_t *save) {
  return vmu_write_now(save, save->data) == 0 ? 0 : -1;
}

/* The background saver. */
static const enj_vmu_save_t *saver_save;
static unsigned saver_delay_ms;
static uint8_t *saver_copy;
static mutex_t saver_lock = MUTEX_INITIALIZER;
static semaphore_t saver_wake = SEM_INITIALIZER(0);
static volatile int saver_dirty;
static volatile uint64_t saver_changed_ms;
static volatile int saver_exit;

/* Low priority, so the maple transfers wait for the game, not the reverse. */
static void *saver_thread(void *arg) {
  unsigned wait = 0;
  (void)arg;
  for (;;) {
    if (wait != 0) {
      sem_wait_timed(&saver_wake, (int)wait);
    } else {
      sem_wait(&saver_wake);
    }
    if (!saver_dirty) {
      wait = 0;
      continue;
    }
    uint64_t now = timer_ms_gettime64();
    if (!saver_exit && now - saver_changed_ms < saver_delay_ms) {
      wait = (unsigned)(saver_delay_ms - (now - saver_changed_ms));
      continue;
    }
    mutex_lock(&saver_lock);
    memcpy(saver_copy, saver_save->data, saver_save->size);
    saver_dirty = 0;
    mutex_unlock(&saver_lock);
    wait = vmu_write_now(saver_save, saver_copy);
    if (wait != 0) {
      saver_dirty = 1; /* try again later */
    }
    if (saver_exit) {
      arch_exit();
    }
  }
  return NULL;
}

void enj_vmu_save_start(const enj_vmu_save_t *save, unsigned delay_ms) {
  if (saver_save != NULL) {
    return;
  }
  saver_copy = memalign(32, save->size);
  if (saver_copy == NULL) {
    return;
  }
  saver_save = save;
  saver_delay_ms = delay_ms;
  kthread_attr_t attr = {0};
  attr.create_detached = true;
  attr.prio = PRIO_DEFAULT + 4; /* a higher number is a lower priority */
  attr.label = "enj_vmu_save";
  thd_create_ex(&attr, saver_thread, NULL);
}

void enj_vmu_save_lock(void) { mutex_lock(&saver_lock); }
void enj_vmu_save_unlock(void) { mutex_unlock(&saver_lock); }

void enj_vmu_save_changed(void) {
  if (saver_save == NULL) {
    return;
  }
  saver_changed_ms = timer_ms_gettime64();
  saver_dirty = 1;
  sem_signal(&saver_wake);
}

int enj_vmu_save_exit_after_flush(void) {
  if (saver_save == NULL || !saver_dirty) {
    return 0;
  }
  saver_exit = 1;
  sem_signal(&saver_wake);
  return 1;
}

#else /* host backends: plain files below /vmu/, written at once */

enj_vmu_load_e enj_vmu_save_load(const enj_vmu_save_t *save) {
  snprintf(vmu_path, sizeof(vmu_path), "/vmu/a1/%s", save->name);
  vmu_checked = 1;
  file_t fd = fs_open(vmu_path, O_RDONLY);
  if (fd < 0) {
    return ENJ_VMU_NO_SAVE;
  }
  off_t bytes = fs_seek(fd, 0, SEEK_END);
  fs_seek(fd, 0, SEEK_SET);
  enj_vmu_load_e result = ENJ_VMU_UNREADABLE;
  if (bytes == (off_t)save->size) {
    uint8_t *file = malloc(save->size);
    if (file != NULL &&
        fs_read(fd, file, save->size) == (ssize_t)save->size) {
      memcpy(save->data, file, save->size);
      result = ENJ_VMU_LOADED;
    }
    free(file);
  }
  fs_close(fd);
  return result;
}

int enj_vmu_save_write(const enj_vmu_save_t *save) {
  if (vmu_path[0] == '\0') {
    snprintf(vmu_path, sizeof(vmu_path), "/vmu/a1/%s", save->name);
  }
  file_t fd = fs_open(vmu_path, O_WRONLY | O_TRUNC);
  if (fd < 0) {
    return -1;
  }
  ssize_t written = fs_write(fd, save->data, save->size);
  return fs_close(fd) == 0 && written == (ssize_t)save->size ? 0 : -1;
}

static const enj_vmu_save_t *saver_save;

void enj_vmu_save_start(const enj_vmu_save_t *save, unsigned delay_ms) {
  (void)delay_ms;
  saver_save = save;
}
void enj_vmu_save_lock(void) {}
void enj_vmu_save_unlock(void) {}
void enj_vmu_save_changed(void) {
  if (saver_save != NULL) {
    (void)enj_vmu_save_write(saver_save);
  }
}
int enj_vmu_save_exit_after_flush(void) { return 0; }

#endif
