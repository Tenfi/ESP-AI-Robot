<template>
  <v-container>
    <div class="portrait-layout d-flex flex-column">
      <v-img :src="currentImage" alt="Image Stream" class="d-flex flex-wrap">
        <div v-if="ws && ws?.readyState !== ws?.OPEN" class="connecting-tip"> Connecting... </div>
        <v-btn @click="toggleFullscreen" :icon="isFullscreen ? mdiFullscreen : mdiFullscreenExit" variant="plain"
          class="fullscreen-btn" />
        <div class="d-flex">
          <v-btn @click="takeScreenshot" :icon="mdiCamera" size="large" variant="tonal"
            style="position: absolute; bottom: 0.5em; left: 50%; transform: translateX(-50%);" />
        </div>
      </v-img>
      <v-row justify="center" class="mt-3 flex-column">
        <v-chip-group v-model="lightMode" selected-class="text-primary" mandatory class="mx-auto">
          <v-chip size="small" v-for="(item, index) in ['A', 'B', 'C']" :key="index" :text="item" :value="index + 2" />
          <span class="mt-1 mr-1">Light Mode</span>
          <v-chip size="small" v-for="(item, index) in ['D', 'E', 'F']" :key="index" :text="item" :value="index + 5" />
        </v-chip-group>
        <div class="mx-auto">
          <v-btn @click="() => sendCData(-0.01)" :icon="mdiSkewLess" variant="tonal" size="x-small" />
          <span class="mt-1 mx-1">Path Correction</span>
          <v-btn @click="() => sendCData(0.01)" :icon="mdiSkewLessMirror" variant="tonal" size="x-small" />
        </div>
      </v-row>
      <div class="d-flex flex-wrap mt-8">
        <Joystick :size="200" base-color="#06060644" stick-color="#66CCFF66" :throttle="100" :base-image="stickImg"
          :stick-size="100" class="mx-auto" @stop="stop" @move="move" />
      </div>
    </div>
    <v-dialog v-model="isUnsupportWS" max-width="800">
      <v-card title="Unsupported Browser" color="red-lighten-2">
        <template #text>
          Sorry, your browser does not support the WebSocket protocol, and this feature cannot be used. Please update
          your browser or try again with the latest version of <a href="https://firefox.com/">Firefox</a>, <a
            href="https://chrome.google.com/">Chrome</a> or other modern browsers.
        </template>
      </v-card>
    </v-dialog>
  </v-container>
</template>

<script setup lang="ts">
  import { ref, onMounted, onUnmounted } from "vue";
  import Joystick from 'vue-joystick-component'
  import { mdiFullscreen, mdiFullscreenExit, mdiCamera, mdiSkewLess } from "@mdi/js";

  const ws = ref<WebSocket | null>(null);
  const currentImage = ref<string>("/assets/loading1200x900-2.jpg");
  const isFullscreen = ref<boolean>(Boolean(!document.fullscreenElement))
  const isUnsupportWS = ref<boolean>(!Boolean("WebSocket" in window))
  const lightMode = ref<number | null>(null)

  const stickImg = '"data:image/svg+xml, %3Csvg%20width%3D%2230mm%22%20height%3D%2230mm%22%20version%3D%221.1%22%20viewBox%3D%220%200%2030%2030%22%20xml%3Aspace%3D%22preserve%22%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%3E%3Cpath%20d%3D%22m15%200a15%2015%200%200%200-15%2015%2015%2015%200%200%200%2015%2015%2015%2015%200%200%200%2015-15%2015%2015%200%200%200-15-15zm0%202.5001%202.5001%202.5001-0.7643%200.76378-1.1048-1.1048v2.8407h-1.2625v-2.8407l-1.1048%201.1048-0.76378-0.76378%202.5001-2.5001zm-9.9999%209.9999%200.76378%200.76378-1.1048%201.1048h2.8407v1.2625h-2.8407l1.1048%201.1048-0.76378%200.7643-2.5001-2.5001%202.5001-2.5001zm20%200%202.5001%202.5001-2.5001%202.5001-0.76378-0.7643%201.1048-1.1048h-2.8412v-1.2625h2.8412l-1.1048-1.1048%200.76378-0.76378zm-10.631%209.9999h1.2625v2.8412l1.1048-1.1048%200.7643%200.76378-2.5001%202.5001-2.5001-2.5001%200.76378-0.76378%201.1048%201.1048v-2.8412z%22%20fill-opacity%3D%22.39875%22%20stroke-width%3D%22.26615%22%2F%3E%3C%2Fsvg%3E"'
  const mdiSkewLessMirror = "M6.5,11L8.59,20H13.5L11.41,11H6.5M4,9H13L16,22H7L4,9M20,6L16,2V5H8V7H16V10L20,6Z"

  let heartbeatInterval: ReturnType<typeof setInterval> | null = null;

  const stop = () => {
    if (ws.value && ws.value.readyState === ws.value.OPEN) {
      ws.value.send(`x0y0`);
    }
  }

  const move = (input: { x?: number, y?: number, direction?: 'LEFT' | 'RIGHT' | 'FORWARD' | 'BACKWARD', distance?: number }) => {
    if (ws.value && ws.value.readyState === ws.value.OPEN) {
      ws.value.send(`x${(input.x ?? 0).toFixed(2)}y${(input.y ?? 0).toFixed(2)}`)
    }
  }

  const takeScreenshot = () => {
    console.log("Screenshot taken");
    const link = document.createElement("a");
    link.href = currentImage.value;
    link.download = `IMG_${new Date().getTime()}.jpg`
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  const sendCData = (n: number) => {
    if (ws.value) {
      ws.value.send(`c${n.toFixed(2)}`)
    }
  }

  const toggleFullscreen = () => {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen();
    } else {
      document.exitFullscreen();
    }
    isFullscreen.value = !isFullscreen.value
  };

  const connectWebSocket = () => {
    ws.value = new WebSocket("/ws");

    ws.value.onopen = () => {
      console.log("WebSocket connected");
    };

    ws.value.onmessage = (event) => {
      if (typeof event.data === "string") {
        console.log("Received message:", event.data);
      } else {
        const blob = new Blob([event.data], { type: "image/jpeg" });
        if (currentImage.value) URL.revokeObjectURL(currentImage.value);
        currentImage.value = URL.createObjectURL(blob);
      }
    };

    ws.value.onerror = (error) => {
      console.error("WebSocket error:", error);
    };

    ws.value.onclose = () => {
      console.log("WebSocket closed");
      setTimeout(connectWebSocket, 3000);
    };
  };

  onMounted(() => {
    connectWebSocket();
  });

  onUnmounted(() => {
    if (heartbeatInterval) clearInterval(heartbeatInterval);
    if (ws.value) ws.value.close();
  });

  const changeLight = () => {
    console.log(`w${lightMode.value}`)
    if (ws.value && ws.value.readyState === ws.value.OPEN && lightMode.value) {
      ws.value.send(`w${lightMode.value}`)
    }
  }

  watch(lightMode, changeLight)

  watch(ws, (newWs, oldWs) => {
    if (heartbeatInterval) {
      clearInterval(heartbeatInterval);
      heartbeatInterval = null;
      console.log('Cleared old heartbeat task');
    }

    if (newWs) {
      console.log('Setting up new heartbeat task');
      heartbeatInterval = setInterval(() => {
        if (newWs.readyState === WebSocket.OPEN) {
          newWs.send('PING');
          console.log('Sent heartbeat: PING');
        } else {
          console.warn('WebSocket is not open, cannot send PING');
        }
      }, 100);
    }
  });
</script>

<style lang="css">
.fullscreen-btn {
  opacity: 50%;
}

.connecting-tip {
  position: absolute;
  top: 1em;
  right: 1em;
  color: red;
}

.portrait-layout .function-buttons {
  margin: 10px 0;
}
</style>