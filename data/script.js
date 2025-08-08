var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// Define variables for all document IDs
const redLedSlider = document.getElementById('redLedSlider');
const redLedValue = document.getElementById('redLedValue');
const redLedDisplay = document.getElementById('redLedDisplay');
const redLedConfirm = document.getElementById('redLedConfirm');
const redLedSwitch = document.getElementById('redLedSwitch');

const blueLedSlider = document.getElementById('blueLedSlider');
const blueLedValue = document.getElementById('blueLedValue');
const blueLedDisplay = document.getElementById('blueLedDisplay');
const blueLedConfirm = document.getElementById('blueLedConfirm');
const blueLedSwitch = document.getElementById('blueLedSwitch');

const warmLedSlider = document.getElementById('warmLedSlider');
const warmLedValue = document.getElementById('warmLedValue');
const warmLedDisplay = document.getElementById('warmLedDisplay');
const warmLedConfirm = document.getElementById('warmLedConfirm');
const warmLedSwitch = document.getElementById('warmLedSwitch');

const coolLedSlider = document.getElementById('coolLedSlider');
const coolLedValue = document.getElementById('coolLedValue');
const coolLedDisplay = document.getElementById('coolLedDisplay');
const coolLedConfirm = document.getElementById('coolLedConfirm');
const coolLedSwitch = document.getElementById('coolLedSwitch');

const temperature = document.getElementById("temperature");
const humidity = document.getElementById("humidity");

const relay1 = document.getElementById("relay1Switch");
const relay2 = document.getElementById("relay2Switch");
const relay3 = document.getElementById("relay3Switch");

const globalSwitch = document.getElementById('globalSwitch');

// Function to sync slider value to display
function syncSliderToDisplay(slider, display) {
    display.textContent = slider.value;
    slider.addEventListener('input', function () {
        display.textContent = slider.value;
        updateSliderPWM(slider);
    });
}

// Function to update slider value from input field
function updateSliderFromInput(slider, display, input, confirmButton) {
    confirmButton.addEventListener('click', function () {
        const inputValue = parseInt(input.value);
        if (inputValue >= 0 && inputValue <= 255) {
            slider.value = inputValue;
            display.textContent = inputValue;
            updateSliderPWM(slider);
        } else {
            alert("Please enter a value between 0 and 255.");
        }
    });
}

// Update slider PWM and send value to the server
function updateSliderPWM(element) {
    var sliderNumber;
    if (element.id.includes('redLed')) sliderNumber = 1;
    else if (element.id.includes('blueLed')) sliderNumber = 2;
    else if (element.id.includes('warmLed')) sliderNumber = 3;
    else if (element.id.includes('coolLed')) sliderNumber = 4;

    var sliderValue = element.value;
    websocket.send(sliderNumber + "s" + sliderValue.toString());
}

// Function to handle relay toggle and send message to server
function handleRelayToggle(relay, relayNumber) {
    relay.addEventListener('change', function () {
        const relayState = relay.checked ? '1' : '0';
        websocket.send(`relay${relayNumber}:${relayState}`);
    });
}

// Function to handle global switch toggle
function handleGlobalSwitch(globalSwitch, relays) {
    globalSwitch.addEventListener('change', function () {
        const globalState = globalSwitch.checked ? '1' : '0';
        
        // Update the relays and LED switches based on global switch state
        relays.forEach(relay => relay.checked = globalSwitch.checked);

        // Send global state message
        websocket.send(`globalSwitch:${globalState}`);
        
        // Send individual relay states as well
        relays.forEach((relay, index) => {
            const relayState = relay.checked ? '1' : '0';
            websocket.send(`relay${index + 1}:${relayState}`);
        });
    });
}

// Initialize WebSocket
function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    websocket.send("sliderValues"); // Request initial values when the connection is opened
    websocket.send("dhtValues"); //Request initial DHT Values
    websocket.send("relayValues");
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Handle incoming WebSocket messages related to relay states
function onMessage(event) {
    var myObj = JSON.parse(event.data);
    if (myObj.redSlider) {
        redLedSlider.value = myObj.redSlider;
        redLedDisplay.textContent = myObj.redSlider;
    }
    if (myObj.blueSlider) {
        blueLedSlider.value = myObj.blueSlider;
        blueLedDisplay.textContent = myObj.blueSlider;
    }
    if (myObj.warmSlider) {
        warmLedSlider.value = myObj.warmSlider;
        warmLedDisplay.textContent = myObj.warmSlider;
    }
    if (myObj.coolSlider) {
        coolLedSlider.value = myObj.coolSlider;
        coolLedDisplay.textContent = myObj.coolSlider;
    }
    if (myObj.temperature) {
        temperature.textContent = myObj.temperature;
    }
    if (myObj.humidity) {
        humidity.textContent = myObj.humidity;
    }
    if (myObj.relay1 !== undefined) {
        relay1.checked = myObj.relay1 === "1"; // Check if relay1 is '1' (on)
    }
    if (myObj.relay2 !== undefined) {
        relay2.checked = myObj.relay2 === "1"; // Check if relay2 is '1' (on)
    }
    if (myObj.relay3 !== undefined) {
        relay3.checked = myObj.relay3 === "1"; // Check if relay3 is '1' (on)
    }
    if (myObj.globalSwitch !== undefined) {
        globalSwitch.checked = myObj.globalSwitch === "1"; // Check if globalSwitch is '1' (on)
    }
}

// Initialize WebSocket and other event handlers
function init() {
    syncSliderToDisplay(redLedSlider, redLedDisplay);
    updateSliderFromInput(redLedSlider, redLedDisplay, redLedValue, redLedConfirm);

    syncSliderToDisplay(blueLedSlider, blueLedDisplay);
    updateSliderFromInput(blueLedSlider, blueLedDisplay, blueLedValue, blueLedConfirm);

    syncSliderToDisplay(warmLedSlider, warmLedDisplay);
    updateSliderFromInput(warmLedSlider, warmLedDisplay, warmLedValue, warmLedConfirm);

    syncSliderToDisplay(coolLedSlider, coolLedDisplay);
    updateSliderFromInput(coolLedSlider, coolLedDisplay, coolLedValue, coolLedConfirm);

    initWebSocket();

    // Relay toggle handlers
    handleRelayToggle(relay1, 1);
    handleRelayToggle(relay2, 2);
    handleRelayToggle(relay3, 3);

    // Global switch handler
    const globalSwitch = document.getElementById('globalSwitch');
    handleGlobalSwitch(globalSwitch, [relay1, relay2, relay3]);
}

// Call init function on page load
window.onload = init;