let socket = new WebSocket(`ws://${window.location.host}/ws`);

socket.binaryType = "arraybuffer";

socket.onopen = function () {
    console.log("WebSocket connection established");
};

socket.onmessage = function (event) {
    if (event.data instanceof ArrayBuffer) {
        console.log("Received binary data: ", new Uint8Array(event.data));
    }
};

function sendBinaryData() {
    let binaryData = new Uint8Array([0x01, 0x02, 0x03, 0x04]);
    socket.send(binaryData);
    console.log("Sent binary data: ", binaryData);
}
