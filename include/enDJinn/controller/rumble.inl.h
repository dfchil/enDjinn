#include <enDJinn/compiledefs.h>
#include <enDJinn/controller/dreamcast.h>
#include <enDJinn/controller/rumble.h>
#include <kos/sem.h>
#include <pthread.h>
#include <stdio.h>

#define RFX_QUEUE_SIZE 0b1111
volatile uint32_t _fx_queues[MAPLE_PORT_COUNT][RFX_QUEUE_SIZE]
    __attribute__((aligned(32))) = {0};
volatile uint32_t _rfx_queue_heads[MAPLE_PORT_COUNT]
    __attribute__((aligned(32))) = {0};
uint32_t _rfx_queue_tails[MAPLE_PORT_COUNT] __attribute__((aligned(32))) = {0};
static pthread_t _rfx_threads[MAPLE_PORT_COUNT] __attribute__((aligned(32)));
static pthread_mutex_t _fx_queue_mutex[MAPLE_PORT_COUNT]
    __attribute__((aligned(32)));
static pthread_cond_t _rfx_queue_cond[MAPLE_PORT_COUNT]
    __attribute__((aligned(32)));

static void *_rfx_consumer(void *arg) {
  int port = (int)(intptr_t)arg;
  while (1) {
    pthread_mutex_lock(_fx_queue_mutex + port);
    pthread_cond_wait(_rfx_queue_cond + port, _fx_queue_mutex + port);
    pthread_mutex_unlock(&_fx_queue_mutex[port]);
    while (_rfx_queue_tails[port] < _rfx_queue_heads[port]) {
      uint32_t rfx =
          _fx_queues[port][(_rfx_queue_tails[port]++) % RFX_QUEUE_SIZE];
      maple_device_t *rumble = maple_port_type(port, MAPLE_FUNC_PURUPURU);
      if (rumble != NULL) {
        // print_rumble_fields((purupuru_effect_t){.raw = rfx});
        purupuru_rumble_raw(rumble, rfx);

        // DEBUG_PRINT("Rumble queue\t%d: tail:\t%d head:\t%d, modulo:\t%d\n",
        //             port, _rfx_queue_tails[port], _rfx_queue_heads[port],
        //             _rfx_queue_heads[port] & RFX_QUEUE_SIZE);
      }
    }
  }
  return NULL;
}

typedef struct rumble_fun {
  union {
    uint32_t raw;
    struct {
      uint32_t vd : 2;
      uint32_t vp : 2;
      uint32_t vn : 4;
      uint32_t va : 4;
      uint32_t owf : 1;
      uint32_t pd : 3;
      uint32_t cv : 1;
      uint32_t of : 1;
      uint16_t fm0_1 : 16;
    } __attribute__((packed));
  };
} rumble_fun_t;

void rumble_queued(uint32_t ctrloffset, uint32_t raw) {
  if (_rfx_queue_heads[ctrloffset] - _rfx_queue_tails[ctrloffset] ==
      RFX_QUEUE_SIZE) {
    DEBUG_PRINT("Rumble queue full, dropping rumble command\n");
    return;
  }
  if (pthread_mutex_trylock(&_fx_queue_mutex[ctrloffset]) == 0) {
    _fx_queues[ctrloffset][_rfx_queue_heads[ctrloffset] % RFX_QUEUE_SIZE] = raw;
    _rfx_queue_heads[ctrloffset]++;
    pthread_cond_signal(_rfx_queue_cond + ctrloffset);
    pthread_mutex_unlock(_fx_queue_mutex + ctrloffset);
  }
}

void rumble_queues_init(void) {
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    _rfx_queue_tails[i] = _rfx_queue_heads[i] = 0;
    pthread_mutex_init(_fx_queue_mutex + i, NULL);
    pthread_cond_init(_rfx_queue_cond + i, NULL);
    pthread_create(_rfx_threads + i, NULL, _rfx_consumer, (void *)(intptr_t)i);
  }
}
void rumble_queues_shutdown(void) {
  for (int i = 0; i < MAPLE_PORT_COUNT; i++) {
    pthread_cancel(_rfx_threads[i]);
    pthread_mutex_destroy(_fx_queue_mutex + i);
    pthread_cond_destroy(&_rfx_queue_cond[i]);
  }
}

void print_rumble_fields(purupuru_effect_t fields) {
  DEBUG_PRINT("Rumble Fields:\n");
  DEBUG_PRINT("-- 1st byte ---------------------------\n");
  DEBUG_PRINT("  .cont  (0)   =  %s,\n", fields.cont ? "true" : "false");
  DEBUG_PRINT("  .res   (1-3)   =  %u,\n", fields.res);
  DEBUG_PRINT("  .motor (4-7)  =  %u,\n", fields.motor);
  DEBUG_PRINT("-- 2nd byte ---------------------------\n");
  DEBUG_PRINT("  .bpow  (0-2)  =  %u,\n", fields.bpow);
  DEBUG_PRINT("  .div   (3)    =  %s,\n", fields.div ? "true" : "false");
  DEBUG_PRINT("  .fpow  (4-6)   =  %u,\n", fields.fpow);
  DEBUG_PRINT("  .conv  (7)    =  %s,\n", fields.conv ? "true" : "false");
  DEBUG_PRINT("-- 3rd byte ---------------------------\n");
  DEBUG_PRINT("  .freq  (0-7)  =  %u,\n", fields.freq);
  DEBUG_PRINT("-- 4th byte ---------------------------\n");
  DEBUG_PRINT("  .inc   (0-7)   =  %u,\n", fields.inc);
  DEBUG_PRINT("-------------------------------------\n");

  unsigned char cmdbits[9 * 4] = {0};
  for (int bidx = 0; bidx < 4; bidx++) {
    for (int j = 0; j < 8; j++) {
      cmdbits[bidx * 9 + (7 - j)] = '0' + ((fields.raw >> (bidx * 8 + j)) & 1);
    }
    cmdbits[(3 - bidx) * 9 + 8] = '\n';
  }
  cmdbits[9 * 4 - 1] = 0;
  DEBUG_PRINT("  four byte pattern:\n%s\n", cmdbits);
  DEBUG_PRINT("-------------------------------------\n");
}