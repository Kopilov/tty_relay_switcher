const int totalRelays = 4;
const int pinoutOffset = 4;

void setup() {
    for (int i = 0; i < totalRelays; i++) {
        pinMode(i + pinoutOffset, OUTPUT);
    }
    Serial.begin(115200);
}

//command example:
//SwitchTheRelay(number = 1, delaySeconds = 3);
String switchRelayCommandBegin = String("SwitchTheRelay(");
String switchRelayCommandEnd = String(");");

bool isSwitchRelayCommand(String &command) {
    return command.startsWith(switchRelayCommandBegin) && command.endsWith(switchRelayCommandEnd);
}

class SwitchRelayCommand {
    public:

    int number;
    int delaySeconds;
    
    SwitchRelayCommand(String commandSrc) {
        commandSrc.remove(0, switchRelayCommandBegin.length());
        commandSrc.remove(commandSrc.length() - switchRelayCommandEnd.length(), switchRelayCommandEnd.length());
        
        int delimiterPosition;
        do {
            delimiterPosition = commandSrc.indexOf(',');
            String token;
            if (delimiterPosition == -1) {
                if (commandSrc.length() == 0) break;
                token = commandSrc;
            } else {
                token = commandSrc.substring(0, delimiterPosition);
                commandSrc = commandSrc.substring(delimiterPosition + 1);
            }

            token.trim();
            token.replace(" ", "");
            token.replace(":", "");
            token.replace("=", "");

            if (token.startsWith("number")) {
                token.replace("number", "");
                number = token.toInt();
            }
            if (token.startsWith("delaySeconds")) {
                token.replace("delaySeconds", "");
                delaySeconds = token.toInt();
            }
        } while (delimiterPosition != -1);        
    }
};

class RelayState {
    public:
    int number;
    int secondsToBeEnabled = 0;

    void enableForSeconds(int seconds) {
        Serial.write("enableForSeconds ");
        Serial.write(itoa(seconds, "        ", 10));
        Serial.write("\r\n");

        if (seconds > 0) {
            secondsToBeEnabled = seconds;
            digitalWrite(number + pinoutOffset, HIGH);
        }
        if (seconds <= 0) {
            disable();
        }
    }

    void disable() {
        secondsToBeEnabled = 0;
        digitalWrite(number + pinoutOffset, LOW);
    }

    void tick() {
        if (secondsToBeEnabled > 0) {
            secondsToBeEnabled--;
            if (secondsToBeEnabled == 0) {
                disable();
            }
        }
    }
};

class RelaysState {
    RelayState relaysStateInternal[totalRelays];
    
    public:
    
    RelaysState() {
        for (int i = 0; i < totalRelays; i++) {
            relaysStateInternal[i].number = i;
        }
    }

    void execCommand(SwitchRelayCommand command) {
        Serial.write("execCommand\r\n");
        if (totalRelays > command.number) {
            Serial.write("command.number ");
            Serial.write(itoa(command.number, "     ", 10));
            Serial.write("\r\n");
            relaysStateInternal[command.number].enableForSeconds(command.delaySeconds);
        }
    }

    void tick() {
        for (int i = 0; i < totalRelays; i++) {
            relaysStateInternal[i].tick();
        }
    }
};

RelaysState currentRelaysState;

void loop() {
    Serial.write("TTY relay switcher is ready\r\n");
    String input = Serial.readString(); //delay 1000 inside
    currentRelaysState.tick();

    input.trim();
    if (input.length() == 0) return;
    
    if (isSwitchRelayCommand(input)) {
        SwitchRelayCommand command = SwitchRelayCommand(input);
        currentRelaysState.execCommand(command);

        Serial.write("Original input:\r\n");
        Serial.write(input.c_str());
        Serial.write("\r\n");
    } else {
        Serial.write("Unknown input:\r\n");
        Serial.write(input.c_str());
        Serial.write("\r\n");
    }
}
