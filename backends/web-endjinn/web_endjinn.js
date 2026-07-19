addOnPreRun(() => {
  FS.mkdir("/vmu");
  FS.mount(IDBFS, {autoPersist: true}, "/vmu");
  addRunDependency("web-endjinn-idbfs");
  FS.syncfs(true, error => {
    if (error) console.error("web-enDjinn: failed to load saves", error);
    removeRunDependency("web-endjinn-idbfs");
  });
});

document.getElementById("resize")?.setAttribute("checked", "");
addOnPostRun(() => {
  const canvas = document.getElementById("canvas");
  Module.requestFullscreen = (lockPointer, resizeCanvas) => {
    if (lockPointer) {
      document.addEventListener(
          "fullscreenchange",
          () => canvas.requestPointerLock()?.catch(() => {}), {once: true});
    }
    Module._web_endjinn_request_fullscreen(resizeCanvas);
  };
});
