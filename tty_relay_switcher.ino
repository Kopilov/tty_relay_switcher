const int totalRelays = 4;
const int pinoutOffset = 7;
const int zeroBasedNumber = false;
const bool pinoutInversed = true;

void setup() {
    for (int i = 0; i < totalRelays; i++) {
        if (pinoutInversed) {
            pinMode(pinoutOffset - i, OUTPUT);
        } else {
            pinMode(pinoutOffset + i, OUTPUT);
        }
    }
    Serial.begin(115200);
}

//command example:
//SwitchTheRelay(number = 4, delaySeconds = 3);
String switchRelayCommandBegin = String("SwitchTheRelay(");
String switchRelayCommandEnd = String(");");

bool isSwitchRelayCommand(String &command) {
    return command.startsWith(switchRelayCommandBegin) && command.endsWith(switchRelayCommandEnd);
}

class SwitchRelayCommand {
    String src;
    public:

    int number;
    int delaySeconds;

    SwitchRelayCommand(String commandSrc) {
        src = String(commandSrc.c_str());
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
                if (!zeroBasedNumber) {
                    number--;
                }
            }
            if (token.startsWith("delaySeconds")) {
                token.replace("delaySeconds", "");
                delaySeconds = token.toInt();
            }
        } while (delimiterPosition != -1);
    }

    String toString() {
        return src;
    }
};

void switchRelay(int number, int level) {
    if (pinoutInversed) {
        digitalWrite(pinoutOffset - number, level);
    } else {
        digitalWrite(pinoutOffset + number, level);
    }
}

class RelayState {
    public:
    int number;
    int secondsToBeEnabled = 0;

    void enableForSeconds(int seconds) {
        if (seconds > 0) {
            secondsToBeEnabled = seconds;
            switchRelay(number, HIGH);
        }
        if (seconds <= 0) {
            disable();
        }
    }

    void disable() {
        secondsToBeEnabled = 0;
        switchRelay(number, LOW);
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
        if (totalRelays > command.number) {
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
        Serial.write("Executing command:\r\n");
        Serial.write(command.toString().c_str());
        currentRelaysState.execCommand(command);
    } else {
        Serial.write("Unknown input:\r\n");
        Serial.write(input.c_str());
    }
    Serial.write("\r\n");
}
