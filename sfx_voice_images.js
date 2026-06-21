
  var Module = typeof Module != 'undefined' ? Module : {};

  if (!Module['expectedDataFileDownloads']) Module['expectedDataFileDownloads'] = 0;
  Module['expectedDataFileDownloads']++;
  (() => {
    // Do not attempt to redownload the virtual filesystem data when in a pthread or a Wasm Worker context.
    var isPthread = typeof ENVIRONMENT_IS_PTHREAD != 'undefined' && ENVIRONMENT_IS_PTHREAD;
    var isWasmWorker = typeof ENVIRONMENT_IS_WASM_WORKER != 'undefined' && ENVIRONMENT_IS_WASM_WORKER;
    if (isPthread || isWasmWorker) return;
    async function loadPackage(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.substring(0, window.location.pathname.lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.substring(0, location.pathname.lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = 'sfx_voice_images.data';
      var REMOTE_PACKAGE_BASE = 'sfx_voice_images.data';
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;
      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];

      async function fetchRemotePackage(packageName, packageSize) {
        
        if (!Module['dataFileDownloads']) Module['dataFileDownloads'] = {};
        try {
          var response = await fetch(packageName);
        } catch (e) {
          throw new Error(`Network Error: ${packageName}`, {e});
        }
        if (!response.ok) {
          throw new Error(`${response.status}: ${response.url}`);
        }

        const chunks = [];
        const headers = response.headers;
        const total = Number(headers.get('Content-Length') || packageSize);
        let loaded = 0;

        Module['setStatus'] && Module['setStatus']('Downloading data...');
        const reader = response.body.getReader();

        while (1) {
          var {done, value} = await reader.read();
          if (done) break;
          chunks.push(value);
          loaded += value.length;
          Module['dataFileDownloads'][packageName] = {loaded, total};

          let totalLoaded = 0;
          let totalSize = 0;

          for (const download of Object.values(Module['dataFileDownloads'])) {
            totalLoaded += download.loaded;
            totalSize += download.total;
          }

          Module['setStatus'] && Module['setStatus'](`Downloading data... (${totalLoaded}/${totalSize})`);
        }

        const packageData = new Uint8Array(chunks.map((c) => c.length).reduce((a, b) => a + b, 0));
        let offset = 0;
        for (const chunk of chunks) {
          packageData.set(chunk, offset);
          offset += chunk.length;
        }
        return packageData.buffer;
      }

      var fetchPromise;
      var fetched = Module['getPreloadedPackage'] && Module['getPreloadedPackage'](REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);

      if (!fetched) {
        // Note that we don't use await here because we want to execute the
        // the rest of this function immediately.
        fetchPromise = fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);
      }

    async function runWithFS(Module) {

      function assert(check, msg) {
        if (!check) throw new Error(msg);
      }
Module['FS_createPath']("/", "assets", true, true);
Module['FS_createPath']("/assets", "audio", true, true);
Module['FS_createPath']("/assets/audio", "sfx", true, true);
Module['FS_createPath']("/assets/audio", "voice", true, true);
Module['FS_createPath']("/assets", "images", true, true);
Module['FS_createPath']("/assets/images", "backgrounds", true, true);
Module['FS_createPath']("/assets/images", "gojo", true, true);
Module['FS_createPath']("/assets/images", "sukuna", true, true);

    for (var file of metadata['files']) {
      var name = file['filename']
      Module['addRunDependency'](`fp ${name}`);
    }

      async function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer.constructor.name === ArrayBuffer.name, 'bad input to processPackageData ' + arrayBuffer.constructor.name);
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          for (var file of metadata['files']) {
            var name = file['filename'];
            var data = byteArray.subarray(file['start'], file['end']);
            // canOwn this data in the filesystem, it is a slice into the heap that will never change
        Module['FS_createDataFile'](name, null, data, true, true, true);
        Module['removeRunDependency'](`fp ${name}`);
          }
          Module['removeRunDependency']('datafile_sfx_voice_images.data');
      }
      Module['addRunDependency']('datafile_sfx_voice_images.data');

      if (!Module['preloadResults']) Module['preloadResults'] = {};

      Module['preloadResults'][PACKAGE_NAME] = {fromCache: false};
      if (!fetched) {
        fetched = await fetchPromise;
      }
      processPackageData(fetched);

    }
    if (Module['calledRun']) {
      runWithFS(Module);
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module['preRun'].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/assets/audio/sfx/gojoDomain.wav", "start": 0, "end": 4460750}, {"filename": "/assets/audio/sfx/gojoHeal.wav", "start": 4460750, "end": 5329180}, {"filename": "/assets/audio/sfx/mahoraga.wav", "start": 5329180, "end": 6328406}, {"filename": "/assets/audio/sfx/punch.wav", "start": 6328406, "end": 6529580}, {"filename": "/assets/audio/sfx/sukunaDomain.wav", "start": 6529580, "end": 9937530}, {"filename": "/assets/audio/sfx/teleport.wav", "start": 9937530, "end": 10198680}, {"filename": "/assets/audio/voice/vo_combo.wav", "start": 10198680, "end": 10323174}, {"filename": "/assets/audio/voice/vo_fight.wav", "start": 10323174, "end": 10436148}, {"filename": "/assets/audio/voice/vo_lost.wav", "start": 10436148, "end": 10556034}, {"filename": "/assets/audio/voice/vo_winner.wav", "start": 10556034, "end": 10680528}, {"filename": "/assets/images/.DS_Store", "start": 10680528, "end": 10686676}, {"filename": "/assets/images/backgrounds/bg1.jpeg", "start": 10686676, "end": 10722750}, {"filename": "/assets/images/backgrounds/bg2.jpeg", "start": 10722750, "end": 10757283}, {"filename": "/assets/images/backgrounds/bg3.jpeg", "start": 10757283, "end": 10795632}, {"filename": "/assets/images/backgrounds/bg4.jpeg", "start": 10795632, "end": 10823685}, {"filename": "/assets/images/backgrounds/menuBg.jpeg", "start": 10823685, "end": 10850042}, {"filename": "/assets/images/gojo/gojo.png", "start": 10850042, "end": 11023497}, {"filename": "/assets/images/gojo/gojoKick.png", "start": 11023497, "end": 11583857}, {"filename": "/assets/images/gojo/gojoPunch.png", "start": 11583857, "end": 12177297}, {"filename": "/assets/images/gojo/gojoStill.png", "start": 12177297, "end": 12700929}, {"filename": "/assets/images/gojo/gojoWalkLeft.png", "start": 12700929, "end": 14400067}, {"filename": "/assets/images/gojo/gojoWalkRight.png", "start": 14400067, "end": 16191052}, {"filename": "/assets/images/gojo/gpunch.png", "start": 16191052, "end": 16465043}, {"filename": "/assets/images/mahoraga.png", "start": 16465043, "end": 16700009}, {"filename": "/assets/images/sukuna/.DS_Store", "start": 16700009, "end": 16706157}, {"filename": "/assets/images/sukuna/sukuna.png", "start": 16706157, "end": 16943381}, {"filename": "/assets/images/sukuna/sukunaKick.png", "start": 16943381, "end": 17533129}, {"filename": "/assets/images/sukuna/sukunaPunch.png", "start": 17533129, "end": 18167489}, {"filename": "/assets/images/sukuna/sukunaStill.png", "start": 18167489, "end": 18872626}, {"filename": "/assets/images/sukuna/sukunaWalkLeft.png", "start": 18872626, "end": 19618685}, {"filename": "/assets/images/sukuna/sukunaWalkRight.png", "start": 19618685, "end": 20340492}], "remote_package_size": 20340492});

  })();
