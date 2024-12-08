import { FFmpeg } from './ffmpeg/index';
import { fetchFile, toBlobURL } from 'https://cdn.jsdelivr.net/npm/@ffmpeg/util@0.12.1/+esm';

const ffmpeg = new FFmpeg();
const videoInput = document.getElementById('videoInput');
const canvas = document.getElementById('frameCanvas');
const ctx = canvas.getContext('2d');
const ws = new WebSocket(`ws://${window.location.host}/ws`);

ws.onopen = () => console.log('WebSocket connected to ESP32');
ws.onerror = (e) => console.error('WebSocket error:', e);
ws.onclose = () => console.log('WebSocket disconnected');

videoInput.addEventListener('change', async (event) => {
    const file = event.target.files[0];
    if (!file) return;

    console.log('Loading FFmpeg...');
    const baseURL = "https://unpkg.com/@ffmpeg/core-mt@0.12.6/dist/esm"
    await ffmpeg.load({
        coreURL: await toBlobURL(`${baseURL}/ffmpeg-core.js`, 'text/javascript'),
        wasmURL: await toBlobURL(`${baseURL}/ffmpeg-core.wasm`, 'application/wasm'),
        workerURL: await toBlobURL(`${baseURL}/ffmpeg-core.worker.js`, 'text/javascript'),
    });
    console.log('Processing video...');

    const reader = new FileReader();
    reader.onload = async (e) => {
        const videoData = new Uint8Array(e.target.result);
        ffmpeg.FS('writeFile', 'input.mp4', videoData);

        // Извлечь кадры с уменьшением размера
        await ffmpeg.run('-i', 'input.mp4', '-vf', 'scale=128:64', '-r', '10', 'frame%d.png');
        const frames = ffmpeg.FS('readdir', '/').filter(file => file.startsWith('frame') && file.endsWith('.png'));

        for (const frame of frames) {
            const frameData = ffmpeg.FS('readFile', frame);
            const blob = new Blob([frameData], { type: 'image/png' });
            const img = new Image();
            img.src = URL.createObjectURL(blob);

            await new Promise(resolve => {
                img.onload = () => {
                    ctx.drawImage(img, 0, 0, 128, 64);
                    const imageData = ctx.getImageData(0, 0, 128, 64);
                    const binaryFrame = toMonoBitmap(imageData);
                    ws.send(binaryFrame);
                    URL.revokeObjectURL(img.src);
                    resolve();
                };
                img.onerror = () => console.error('Error loading image:', img.src);
            });

            await new Promise(resolve => setTimeout(resolve, 100)); // Задержка между кадрами
        }

        console.log('All frames sent!');
    };
    reader.readAsArrayBuffer(file);
});

// Преобразование ImageData в монохромный битмап для OLED
function toMonoBitmap(imageData) {
    const { data, width, height } = imageData;
    const buffer = new Uint8Array((width * height) / 8); // Один байт на 8 пикселей

    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const idx = (y * width + x) * 4; // RGBA индекс
            const pixelValue = data[idx] > 128 ? 1 : 0; // Чёрно-белый порог
            const byteIdx = (y * width + x) >> 3; // Индекс байта
            const bitIdx = x & 7; // Позиция бита
            buffer[byteIdx] |= pixelValue << (7 - bitIdx); // Устанавливаем бит
        }
    }
    return buffer;
}