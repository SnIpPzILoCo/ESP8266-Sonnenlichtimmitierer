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

const relay1StartHours = document.getElementById("relay1StartHours");
const relay2StartHours = document.getElementById("relay2StartHours");
const relay3StartHours = document.getElementById("relay3StartHours");

const relay1StartMinutes = document.getElementById("relay1StartMinutes");
const relay2StartMinutes = document.getElementById("relay2StartMinutes");
const relay3StartMinutes = document.getElementById("relay3StartMinutes");

const relay1EndHours = document.getElementById("relay1EndHours");
const relay2EndHours = document.getElementById("relay2EndHours");
const relay3EndHours = document.getElementById("relay3EndHours");

const relay1EndMinutes = document.getElementById("relay1EndMinutes");
const relay2EndMinutes = document.getElementById("relay2EndMinutes");
const relay3EndMinutes = document.getElementById("relay3EndMinutes");

const relay1TimeSwitch = document.getElementById("relay1TimeSwitch");
const relay2TimeSwitch = document.getElementById("relay2TimeSwitch");
const relay3TimeSwitch = document.getElementById("relay3TimeSwitch");

const relay1TimeConfirm = document.getElementById("relay1TimeConfirm");
const relay2TimeConfirm = document.getElementById("relay2TimeConfirm");
const relay3TimeConfirm = document.getElementById("relay3TimeConfirm");

const globalSwitch = document.getElementById('sliderSwitch');

// Function to limit the input values to the given min max in HTML
function validateInputMinMax(inputElement) {
    const min = parseFloat(inputElement.min);
    const max = parseFloat(inputElement.max);
    const value = parseFloat(inputElement.value);
    if (value < min) {
        inputElement.value = min;
    }
    if (value > max) {
        inputElement.value = max;
    }
}

// Function to sync slider value to display
function syncSliderToDisplay(slider, display) {
    if (slider && display) {
        display.textContent = slider.value;
        slider.addEventListener('input', function () {
            display.textContent = slider.value;
            updateSliderPWM(slider);
        });
    }
}

// Function to update slider value from input field
function updateSliderFromInput(slider, display, input, confirmButton) {
    if (slider && display && input && confirmButton) {
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
}

// Update slider PWM and send value to the server
function updateSliderPWM(element) {
    if (websocket) {
        var sliderNumber;
        if (element.id.includes('redLed')) sliderNumber = 1;
        else if (element.id.includes('blueLed')) sliderNumber = 2;
        else if (element.id.includes('warmLed')) sliderNumber = 3;
        else if (element.id.includes('coolLed')) sliderNumber = 4;

        var sliderValue = element.value;
        websocket.send(sliderNumber + "s" + sliderValue.toString());
    }
}

// Function to handle individual LED switches
function handleLedSwitch(switchElement, switchNumber) {
    if (switchElement) {
        switchElement.addEventListener('change', function() {
            const switchState = switchElement.checked ? '1' : '0';
            if (websocket) {
                websocket.send(`rs${switchNumber}:${switchState}`);
            }
        });
    }
}

// Function to handle global switch
function handleGlobalSwitch(globalSwitch) {
    if (globalSwitch) {
        globalSwitch.addEventListener('change', function() {
            const switchState = globalSwitch.checked ? '1' : '0';
            if (websocket) {
                websocket.send(`rG:${switchState}`);
            }
        });
    }
}

// Function to handle relay toggle
function handleRelayToggle(relay, relayNumber) {
    if (relay) {
        relay.addEventListener('change', function () {
            const relayState = relay.checked ? '1' : '0';
            if (websocket) {
                websocket.send(`r${relayNumber}:${relayState}`);
            }
        });
    }
}

function validateTimeInput(element) {
    const value = parseInt(element.value);
    const min = parseInt(element.min);
    const max = parseInt(element.max);
    
    if (isNaN(value) || value < min) {
        element.value = min;
    } else if (value > max) {
        element.value = max;
    }
    
    // Ensure two digits
    element.value = element.value.padStart(2, '0');
}

// Add time input validation to all time inputs
function initializeTimeInputs() {
    const timeInputs = [
        relay1StartHours, relay1StartMinutes,
        relay1EndHours, relay1EndMinutes,
        relay2StartHours, relay2StartMinutes,
        relay2EndHours, relay2EndMinutes,
        relay3StartHours, relay3StartMinutes,
        relay3EndHours, relay3EndMinutes
    ];

    timeInputs.forEach(input => {
        if (input) {
            // Validate on blur
            input.addEventListener('blur', () => {
                validateTimeInput(input);
            });
            
            // Prevent non-numeric input
            input.addEventListener('keypress', (e) => {
                if (!/[0-9]/.test(e.key)) {
                    e.preventDefault();
                }
            });
            
            // Initialize with padded value
            if (input.value) {
                validateTimeInput(input);
            }
        }
    });
}

// Function to compare times and check if they're valid
function isValidTimeRange(startHours, startMinutes, endHours, endMinutes) {
    const start = parseInt(startHours) * 60 + parseInt(startMinutes);
    const end = parseInt(endHours) * 60 + parseInt(endMinutes);
    return start < end;
}



// Enhanced relay time control function
function setRelayTime(relayNumber, startHourElement, startMinuteElement, endHourElement, endMinuteElement, onOffSwitch, confirmButton) {
    if (startHourElement && startMinuteElement && endHourElement && endMinuteElement && onOffSwitch && confirmButton) {
        confirmButton.addEventListener('click', () => {
            // Validate all inputs first
            validateTimeInput(startHourElement);
            validateTimeInput(startMinuteElement);
            validateTimeInput(endHourElement);
            validateTimeInput(endMinuteElement);
            
            const startHour = startHourElement.value.padStart(2, '0');
            const startMinute = startMinuteElement.value.padStart(2, '0');
            const endHour = endHourElement.value.padStart(2, '0');
            const endMinute = endMinuteElement.value.padStart(2, '0');
            
            // Check if time range is valid
            if (!isValidTimeRange(startHour, startMinute, endHour, endMinute)) {
                alert("End time must be after start time");
                return;
            }
            
            const onOffState = onOffSwitch.checked ? "1" : "0";
            // Modified message format to match backend expectations
            const message = `relayTime${relayNumber}:${startHour}:${startMinute}:${endHour}:${endMinute}:${onOffState}`;
            
            if (websocket && websocket.readyState === WebSocket.OPEN) {
                websocket.send(message);
                
                // Provide visual feedback
                confirmButton.classList.add('confirmed');
                setTimeout(() => {
                    confirmButton.classList.remove('confirmed');
                }, 1000);
            } else {
                alert("WebSocket connection is not available. Please try again.");
            }
        });
    }
}

// Function to sync relay time UI with received server state
function updateRelayTimeUI(relayNumber, startHour, startMinute, endHour, endMinute, timeEnabled) {
    const elements = {
        1: {
            startHours: relay1StartHours,
            startMinutes: relay1StartMinutes,
            endHours: relay1EndHours,
            endMinutes: relay1EndMinutes,
            timeSwitch: relay1TimeSwitch
        },
        2: {
            startHours: relay2StartHours,
            startMinutes: relay2StartMinutes,
            endHours: relay2EndHours,
            endMinutes: relay2EndMinutes,
            timeSwitch: relay2TimeSwitch
        },
        3: {
            startHours: relay3StartHours,
            startMinutes: relay3StartMinutes,
            endHours: relay3EndHours,
            endMinutes: relay3EndMinutes,
            timeSwitch: relay3TimeSwitch
        }
    };

    const relay = elements[relayNumber];
    if (relay) {
        // Pad values with leading zeros
        relay.startHours.value = startHour.padStart(2, '0');
        relay.startMinutes.value = startMinute.padStart(2, '0');
        relay.endHours.value = endHour.padStart(2, '0');
        relay.endMinutes.value = endMinute.padStart(2, '0');
        relay.timeSwitch.checked = timeEnabled === "1";
    }
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
    websocket.send("sliderValues");
    websocket.send("dhtValues");
    websocket.send("relayValues");
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    var myObj = JSON.parse(event.data);
    
    // Handle slider values
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
    
    // Handle sensor values
    if (myObj.temperature) {
        temperature.textContent = myObj.temperature;
    }
    if (myObj.humidity) {
        humidity.textContent = myObj.humidity;
    }
    
    // Handle relay states
    if (myObj.relay1 !== undefined) {
        relay1.checked = myObj.relay1 === "1";
    }
    if (myObj.relay2 !== undefined) {
        relay2.checked = myObj.relay2 === "1";
    }
    if (myObj.relay3 !== undefined) {
        relay3.checked = myObj.relay3 === "1";
    }
    
    // Handle LED switch states
    if (myObj.redLedSwitch !== undefined) {
        redLedSwitch.checked = myObj.redLedSwitch === "1";
    }
    if (myObj.blueLedSwitch !== undefined) {
        blueLedSwitch.checked = myObj.blueLedSwitch === "1";
    }
    if (myObj.warmLedSwitch !== undefined) {
        warmLedSwitch.checked = myObj.warmLedSwitch === "1";
    }
    if (myObj.coolLedSwitch !== undefined) {
        coolLedSwitch.checked = myObj.coolLedSwitch === "1";
    }
    
    // Handle global switch state
    if (myObj.globalSwitch !== undefined) {
        globalSwitch.checked = myObj.globalSwitch === "1";
    }

    // Fixed relay time handling
    // Handle relay states separately from time settings
    if (myObj.hasOwnProperty('relay1State')) {
        relay1.checked = myObj.relay1State === "1";
    }
    if (myObj.hasOwnProperty('relay2State')) {
        relay2.checked = myObj.relay2State === "1";
    }
    if (myObj.hasOwnProperty('relay3State')) {
        relay3.checked = myObj.relay3State === "1";
    }
    
    // Handle timer settings separately
    if (myObj.hasOwnProperty('relay1Timer')) {
        const [startHour, startMinute, endHour, endMinute, enabled] = myObj.relay1Timer.split(':');
        updateRelayTimeUI(1, startHour, startMinute, endHour, endMinute, enabled);
    }
}

// Initialize all functionality
function init() {
    // Initialize sliders
    syncSliderToDisplay(redLedSlider, redLedDisplay);
    updateSliderFromInput(redLedSlider, redLedDisplay, redLedValue, redLedConfirm);

    syncSliderToDisplay(blueLedSlider, blueLedDisplay);
    updateSliderFromInput(blueLedSlider, blueLedDisplay, blueLedValue, blueLedConfirm);

    syncSliderToDisplay(warmLedSlider, warmLedDisplay);
    updateSliderFromInput(warmLedSlider, warmLedDisplay, warmLedValue, warmLedConfirm);

    syncSliderToDisplay(coolLedSlider, coolLedDisplay);
    updateSliderFromInput(coolLedSlider, coolLedDisplay, coolLedValue, coolLedConfirm);

    // Initialize LED switches
    handleLedSwitch(redLedSwitch, 1);
    handleLedSwitch(blueLedSwitch, 2);
    handleLedSwitch(warmLedSwitch, 3);
    handleLedSwitch(coolLedSwitch, 4);

    // Initialize global switch
    handleGlobalSwitch(globalSwitch);

    // Initialize relays
    handleRelayToggle(relay1, 1);
    handleRelayToggle(relay2, 2);
    handleRelayToggle(relay3, 3);

    // Initialize time inputs
    initializeTimeInputs();

    // Initialize relay timers with enhanced time control
    setRelayTime(1, relay1StartHours, relay1StartMinutes, relay1EndHours, relay1EndMinutes, relay1TimeSwitch, relay1TimeConfirm);
    setRelayTime(2, relay2StartHours, relay2StartMinutes, relay2EndHours, relay2EndMinutes, relay2TimeSwitch, relay2TimeConfirm);
    setRelayTime(3, relay3StartHours, relay3StartMinutes, relay3EndHours, relay3EndMinutes, relay3TimeSwitch, relay3TimeConfirm);
    
    // Initialize WebSocket connection
    initWebSocket();
}

// Start initialization when page loads
window.addEventListener('load', init);