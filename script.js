// Replace with your BLE Service and Characteristic UUIDs
const serviceUuid = "c34457f2-f9e6-11ee-8020-325096b39f47"; // Replace with your UUID
const characteristicUuid = "abcd5678-efgh-1234-5678-abcd5678efgh"; // Replace with your UUID

const connectButton = document.getElementById('connectButton');
const launchButton = document.getElementById('launchButton');
const abortButton = document.getElementById('abortButton');
const statusDiv = document.getElementById('status');

let device;
let characteristic;

// Connect to Bluetooth device
connectButton.addEventListener('click', () => {
    navigator.bluetooth.requestDevice({ 
        filters: [{ services: [serviceUuid] }],
        optionalServices: [serviceUuid], 
        acceptAllDevices: false 
    })
    .then(dev => {
        device = dev;
        return device.gatt.connect(); 
    })
    .then(server => server.getPrimaryService(serviceUuid))
    .then(service => service.getCharacteristic(characteristicUuid))
    .then(char => {
        characteristic = char;
        statusDiv.textContent = 'Connected!';
        connectButton.disabled = true;
        launchButton.disabled = false;
        abortButton.disabled = false;
    })
    .catch(error => {
        console.error(error);
        statusDiv.textContent = 'Error connecting!'; 
    });
});

// Send "Launch" command
launchButton.addEventListener('click', () => {
    sendData('Launch');  
});

// Send "Abort" command
abortButton.addEventListener('click', () => {
    sendData('Abort'); 
});

// Function to send data over Bluetooth
function sendData(data) {
    if (!characteristic) {
        statusDiv.textContent = 'Not connected!';
        return;
    }
    const encoder = new TextEncoder(); 
    characteristic.writeValue(encoder.encode(data))
    .catch(error => { 
        console.error('Error sending data:', error);
        statusDiv.textContent = 'Error sending command!'; 
    });
}
