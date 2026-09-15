#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <algorithm>
#include <cmath>

constexpr int SCREEN_WIDTH = 1200;
constexpr int SCREEN_HEIGHT = 800;
constexpr int ALPHABET_SIZE = 26;

const std::array<std::string, 10> ROTOR_WIRINGS = {
    "EKMFLGDQVZNTOWYHXUSPAIBRCJ",
    "AJDKSIRUXBLHWTMCQGZNPYFVOE",
    "BDFHJLCPRTXVZNYEIWGAKMUSQO",
    "ESOVPZJAYQUIRHXLNFTGKDCMWB",
    "VZBRGITYUPSDNHLXAWMJQOFECK",
    "JPGVOUMFYQBENHZRDKASXLICTW",
    "NZJHGRCXMYSWBOUFAIVLPEKQDT",
    "FKQHTLXOCBJSPDZRAMEWNIUYGV",
    "LEYJVCNIXWPBQMDRTAKZGFUHOS",
    "FSOKANUERHMBTIYCWLQPZXVGJD"
};

const std::array<const char*, 10> ROTOR_NAMES = {
    "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "Beta", "Gamma"
};

const std::array<std::string, 10> ROTOR_NOTCHES = {
    "Q", "E", "V", "J", "Z", "ZM", "ZM", "ZM", "", ""
};

const std::array<std::string, 5> REFLECTOR_WIRINGS = {
    "EJMZALYXVBWFCRQUONTSPIKHGD",
    "YRUHQSLDPXNGOKMIEBFZCWVJAT",
    "FVPJIAOYEDRZXWGCTKUQSBNMHL",
    "ENKQAUYWJICOPBLMDXZVFTHRGS",
    "RDOBJNTKVEHMLFCWZAXGYIPSUQ"
};

const std::array<const char*, 5> REFLECTOR_NAMES = {
    "UKW-A", "UKW-B", "UKW-C", "UKW-B thin", "UKW-C thin"
};

const std::array<Color, 13> PLUG_COLORS = { {
    {230, 100, 100, 255},
    {100, 180, 100, 255},
    {100, 150, 230, 255},
    {230, 180, 80, 255},
    {180, 100, 220, 255},
    {100, 200, 200, 255},
    {220, 130, 180, 255},
    {160, 200, 100, 255},
    {200, 150, 120, 255},
    {120, 180, 220, 255},
    {220, 200, 100, 255},
    {150, 130, 200, 255},
    {180, 220, 180, 255}
} };

enum MachineType {
    MACHINE_ENIGMA_I = 0,
    MACHINE_M3 = 1,
    MACHINE_M4 = 2
};

const std::array<const char*, 3> MACHINE_NAMES = {
    "Enigma I",
    "M3 Navy",
    "M4 U-boat"
};

struct Rotor {
    std::array<int, ALPHABET_SIZE> wiring;
    std::array<int, ALPHABET_SIZE> inverse;
    int position = 0;
    int ringSetting = 0;
    int rotorType = 0;
    std::string notches;

    void init(int type) {
        rotorType = type;
        position = 0;
        ringSetting = 0;
        notches = ROTOR_NOTCHES[type];
        const std::string& w = ROTOR_WIRINGS[type];
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            wiring[i] = w[i] - 'A';
            inverse[wiring[i]] = i;
        }
    }

    bool atNotch() const {
        return notches.find((char)('A' + position)) != std::string::npos;
    }

    int forward(int input) const {
        int offset = position - ringSetting;
        int entry = (input + offset + 26) % 26;
        int exitVal = wiring[entry];
        return (exitVal - offset + 26) % 26;
    }

    int backward(int input) const {
        int offset = position - ringSetting;
        int entry = (input + offset + 26) % 26;
        int exitVal = inverse[entry];
        return (exitVal - offset + 26) % 26;
    }
};

struct Reflector {
    std::array<int, ALPHABET_SIZE> wiring;
    int type = 1;

    void init(int t) {
        type = t;
        const std::string& w = REFLECTOR_WIRINGS[t];
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            wiring[i] = w[i] - 'A';
        }
    }

    int reflect(int input) const {
        return wiring[input];
    }
};

struct Plugboard {
    std::array<int, ALPHABET_SIZE> mapping;
    std::array<int, ALPHABET_SIZE> pairIndex;
    int numPairs = 0;

    void rebuildPairIndices() {
        pairIndex.fill(-1);
        numPairs = 0;
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (mapping[i] > i) {
                pairIndex[i] = numPairs;
                pairIndex[mapping[i]] = numPairs;
                numPairs++;
            }
        }
    }

    void init() {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            mapping[i] = i;
        }
        rebuildPairIndices();
    }

    void setPair(int a, int b) {
        if (a < 0 || a >= ALPHABET_SIZE || b < 0 || b >= ALPHABET_SIZE) return;
        if (a == b) {
            clearLetter(a);
            return;
        }

        int oldA = mapping[a];
        int oldB = mapping[b];
        if (oldA != a) {
            mapping[oldA] = oldA;
            mapping[a] = a;
        }
        if (oldB != b) {
            mapping[oldB] = oldB;
            mapping[b] = b;
        }

        mapping[a] = b;
        mapping[b] = a;
        rebuildPairIndices();
    }

    void clearLetter(int letter) {
        if (letter < 0 || letter >= ALPHABET_SIZE) return;
        int paired = mapping[letter];
        if (paired != letter) {
            mapping[paired] = paired;
            mapping[letter] = letter;
            rebuildPairIndices();
        }
    }

    int process(int input) const {
        return mapping[input];
    }

    int countPairs() const {
        return numPairs;
    }

    Color getPairColor(int letter) const {
        if (pairIndex[letter] >= 0 && pairIndex[letter] < 13) {
            return PLUG_COLORS[pairIndex[letter] % 13];
        }
        return Color{ 40, 40, 40, 255 };
    }
};

struct EncodingPath {
    std::vector<int> steps;
    int inputLetter = -1;
    int outputLetter = -1;

    void clear() {
        steps.clear();
        inputLetter = -1;
        outputLetter = -1;
    }
};

struct EnigmaMachine {
    MachineType type = MACHINE_ENIGMA_I;
    std::vector<Rotor> rotors;
    Reflector reflector;
    Plugboard plugboard;

    void init(MachineType t) {
        type = t;
        rotors.clear();
        plugboard.init();

        switch (t) {
        case MACHINE_ENIGMA_I:
            rotors.resize(3);
            rotors[0].init(0);
            rotors[1].init(1);
            rotors[2].init(2);
            reflector.init(1);
            break;
        case MACHINE_M3:
            rotors.resize(3);
            rotors[0].init(0);
            rotors[1].init(1);
            rotors[2].init(2);
            reflector.init(1);
            break;
        case MACHINE_M4:
            rotors.resize(4);
            rotors[0].init(8);
            rotors[1].init(0);
            rotors[2].init(1);
            rotors[3].init(2);
            reflector.init(3);
            break;
        }
    }

    void stepRotors() {
        int n = rotors.size();
        int fast = n - 1;
        int middle = n - 2;
        int slow = n - 3;

        bool middleAtNotch = rotors[middle].atNotch();
        bool fastAtNotch = rotors[fast].atNotch();

        rotors[fast].position = (rotors[fast].position + 1) % 26;

        if (fastAtNotch || middleAtNotch) {
            rotors[middle].position = (rotors[middle].position + 1) % 26;
        }

        if (middleAtNotch) {
            rotors[slow].position = (rotors[slow].position + 1) % 26;
        }
    }

    int encode(int input, EncodingPath& path) {
        path.clear();
        path.inputLetter = input;
        stepRotors();

        int current = input;
        path.steps.push_back(current);
        current = plugboard.process(current);
        path.steps.push_back(current);

        for (int i = rotors.size() - 1; i >= 0; i--) {
            current = rotors[i].forward(current);
            path.steps.push_back(current);
        }

        current = reflector.reflect(current);
        path.steps.push_back(current);

        for (size_t i = 0; i < rotors.size(); i++) {
            current = rotors[i].backward(current);
            path.steps.push_back(current);
        }

        path.steps.push_back(current);
        current = plugboard.process(current);
        path.steps.push_back(current);
        path.outputLetter = current;
        return current;
    }
};

struct AppState {
    EnigmaMachine machine;
    EncodingPath currentPath;

    bool animating = false;
    float animationTime = 0.0f;
    float animationSpeed = 1.0f;
    int animationStep = 0;

    std::string inputText;
    std::string outputText;
    std::vector<std::vector<int>> rotorPositionHistory;

    int plugboardFirst = -1;
    int activeDropdown = -1;
};

Font appFont;

void DrawTextF(const char* text, int x, int y, int fontSize, Color color) {
    DrawTextEx(appFont, text, Vector2{ (float)x, (float)y }, fontSize, 1, color);
}

int MeasureTextF(const char* text, int fontSize) {
    return (int)MeasureTextEx(appFont, text, fontSize, 1).x;
}

void DrawLetterCircle(int x, int y, int radius, char letter, Color bg, Color fg, bool highlight) {
    if (highlight) {
        DrawCircle(x, y, radius + 3, YELLOW);
    }
    DrawCircle(x, y, radius, bg);
    DrawCircleLines(x, y, radius, fg);

    const char str[2] = { letter, '\0' };
    int textWidth = MeasureTextF(str, 18);
    DrawTextF(str, x - textWidth / 2, y - 9, 18, fg);
}

void DrawKeyboard(int x, int y, int pressedKey) {
    const char* row1 = "QWERTZUIO";
    const char* row2 = "ASDFGHJK";
    const char* row3 = "PYXCVBNML";

    int keySize = 34;
    int keyGap = 4;
    int keyStep = keySize + keyGap;

    DrawTextF("keyboard", x, y, 14, GRAY);
    y += 22;

    for (int i = 0; i < 9; i++) {
        int kx = x + i * keyStep;
        char key = row1[i];
        bool isPressed = ((key - 'A') == pressedKey);

        Color bg = isPressed ? YELLOW : Color{ 45, 45, 50, 255 };
        Color fg = isPressed ? BLACK : WHITE;

        DrawRectangleRounded(Rectangle{ (float)kx, (float)y, (float)keySize, (float)keySize }, 0.2f, 4, bg);
        DrawRectangleRoundedLines(Rectangle{ (float)kx, (float)y, (float)keySize, (float)keySize }, 0.2f, 4, Color{ 70,70,75,255 });

        const char str[2] = { key, '\0' };
        int tw = MeasureTextF(str, 18);
        DrawTextF(str, kx + keySize / 2 - tw / 2, y + keySize / 2 - 9, 18, fg);
    }

    for (int i = 0; i < 8; i++) {
        int kx = x + (keyStep / 2) + i * keyStep;
        int ky = y + keyStep;
        char key = row2[i];
        bool isPressed = ((key - 'A') == pressedKey);

        Color bg = isPressed ? YELLOW : Color{ 45, 45, 50, 255 };
        Color fg = isPressed ? BLACK : WHITE;

        DrawRectangleRounded(Rectangle{ (float)kx, (float)ky, (float)keySize, (float)keySize }, 0.2f, 4, bg);
        DrawRectangleRoundedLines(Rectangle{ (float)kx, (float)ky, (float)keySize, (float)keySize }, 0.2f, 4, Color{ 70,70,75,255 });

        const char str[2] = { key, '\0' };
        int tw = MeasureTextF(str, 18);
        DrawTextF(str, kx + keySize / 2 - tw / 2, ky + keySize / 2 - 9, 18, fg);
    }

    for (int i = 0; i < 9; i++) {
        int kx = x + i * keyStep;
        int ky = y + keyStep * 2;
        char key = row3[i];
        bool isPressed = ((key - 'A') == pressedKey);

        Color bg = isPressed ? YELLOW : Color{ 45, 45, 50, 255 };
        Color fg = isPressed ? BLACK : WHITE;

        DrawRectangleRounded(Rectangle{ (float)kx, (float)ky, (float)keySize, (float)keySize }, 0.2f, 4, bg);
        DrawRectangleRoundedLines(Rectangle{ (float)kx, (float)ky, (float)keySize, (float)keySize }, 0.2f, 4, Color{ 70,70,75,255 });

        const char str[2] = { key, '\0' };
        int tw = MeasureTextF(str, 18);
        DrawTextF(str, kx + keySize / 2 - tw / 2, ky + keySize / 2 - 9, 18, fg);
    }
}

void DrawLampboard(int x, int y, int litLamp) {
    const char* row1 = "QWERTZUIO";
    const char* row2 = "ASDFGHJK";
    const char* row3 = "PYXCVBNML";

    int lampSize = 30;
    int lampGap = 8;
    int lampStep = lampSize + lampGap;

    DrawTextF("lampboard", x, y, 14, GRAY);
    y += 22;

    for (int i = 0; i < 9; i++) {
        int lx = x + i * lampStep + lampSize / 2;
        int ly = y + lampSize / 2;
        char lamp = row1[i];
        bool isLit = ((lamp - 'A') == litLamp);

        Color bg = isLit ? YELLOW : Color{ 18, 18, 20, 255 };
        Color fg = isLit ? BLACK : Color{ 45, 45, 50, 255 };

        if (isLit) DrawCircle(lx, ly, lampSize / 2 + 4, Fade(YELLOW, 0.4f));
        DrawCircle(lx, ly, lampSize / 2, bg);
        DrawCircleLines(lx, ly, lampSize / 2, isLit ? ORANGE : Color{ 35,35,40,255 });

        const char str[2] = { lamp, '\0' };
        int tw = MeasureTextF(str, 16);
        DrawTextF(str, lx - tw / 2, ly - 8, 16, fg);
    }

    int row2Offset = lampStep / 2;
    for (int i = 0; i < 8; i++) {
        int lx = x + row2Offset + i * lampStep + lampSize / 2;
        int ly = y + lampStep + lampSize / 2;
        char lamp = row2[i];
        bool isLit = ((lamp - 'A') == litLamp);

        Color bg = isLit ? YELLOW : Color{ 18, 18, 20, 255 };
        Color fg = isLit ? BLACK : Color{ 45, 45, 50, 255 };

        if (isLit) DrawCircle(lx, ly, lampSize / 2 + 4, Fade(YELLOW, 0.4f));
        DrawCircle(lx, ly, lampSize / 2, bg);
        DrawCircleLines(lx, ly, lampSize / 2, isLit ? ORANGE : Color{ 35,35,40,255 });

        const char str[2] = { lamp, '\0' };
        int tw = MeasureTextF(str, 16);
        DrawTextF(str, lx - tw / 2, ly - 8, 16, fg);
    }

    for (int i = 0; i < 9; i++) {
        int lx = x + i * lampStep + lampSize / 2;
        int ly = y + lampStep * 2 + lampSize / 2;
        char lamp = row3[i];
        bool isLit = ((lamp - 'A') == litLamp);

        Color bg = isLit ? YELLOW : Color{ 18, 18, 20, 255 };
        Color fg = isLit ? BLACK : Color{ 45, 45, 50, 255 };

        if (isLit) DrawCircle(lx, ly, lampSize / 2 + 4, Fade(YELLOW, 0.4f));
        DrawCircle(lx, ly, lampSize / 2, bg);
        DrawCircleLines(lx, ly, lampSize / 2, isLit ? ORANGE : Color{ 35,35,40,255 });

        const char str[2] = { lamp, '\0' };
        int tw = MeasureTextF(str, 16);
        DrawTextF(str, lx - tw / 2, ly - 8, 16, fg);
    }
}

void DrawRotorWindows(const EnigmaMachine& machine, int x, int y) {
    DrawTextF("rotors", x, y, 14, GRAY);
    y += 22;

    int windowWidth = 48;
    int windowHeight = 60;
    int spacing = 8;

    for (size_t i = 0; i < machine.rotors.size(); i++) {
        int wx = x + i * (windowWidth + spacing);

        DrawRectangle(wx, y, windowWidth, windowHeight, Color{ 12, 12, 15, 255 });
        DrawRectangleLinesEx(Rectangle{ (float)wx, (float)y, (float)windowWidth, (float)windowHeight }, 2, Color{ 160, 130, 70, 255 });

        char letter = 'A' + machine.rotors[i].position;
        const char str[2] = { letter, '\0' };
        int tw = MeasureTextF(str, 32);
        DrawTextF(str, wx + windowWidth / 2 - tw / 2, y + 6, 32, Color{ 120, 170, 100, 255 });

        int labelW = MeasureTextF(ROTOR_NAMES[machine.rotors[i].rotorType], 10);
        DrawTextF(ROTOR_NAMES[machine.rotors[i].rotorType], wx + windowWidth / 2 - labelW / 2, y + 44, 10, GRAY);
    }
}

void DrawPlugboard(const Plugboard& pb, int x, int y, int highlightIn, int highlightOut, int selectedFirst) {
    DrawTextF("plugboard", x, y, 14, GRAY);

    char pairStr[32];
    snprintf(pairStr, sizeof(pairStr), "(%d/13 pairs)", pb.countPairs());
    DrawTextF(pairStr, x + 90, y, 12, Color{ 80,80,85,255 });

    y += 22;

    int panelWidth = 450;
    int panelHeight = 100;
    DrawRectangle(x, y, panelWidth, panelHeight, Color{ 18, 25, 18, 255 });
    DrawRectangleLines(x, y, panelWidth, panelHeight, Color{ 35, 45, 35, 255 });

    int circleRadius = 13;
    int spacing = 33;
    int startX = x + 18;
    int row1Y = y + 26;
    int row2Y = y + 72;

    for (int i = 0; i < 26; i++) {
        if (pb.mapping[i] > i) {
            int paired = pb.mapping[i];

            int row1 = i / 13;
            int col1 = i % 13;
            int x1 = startX + col1 * spacing;
            int y1 = (row1 == 0) ? row1Y : row2Y;

            int row2 = paired / 13;
            int col2 = paired % 13;
            int x2 = startX + col2 * spacing;
            int y2 = (row2 == 0) ? row1Y : row2Y;

            Color lineCol = pb.getPairColor(i);
            if (i == highlightIn || paired == highlightIn) lineCol = YELLOW;

            if (row1 == row2) {
                Vector2 start = { (float)x1, (float)(y1 + ((row1 == 0) ? circleRadius : -circleRadius)) };
                Vector2 end = { (float)x2, (float)(y2 + ((row2 == 0) ? circleRadius : -circleRadius)) };
                Vector2 ctrl = { (start.x + end.x) / 2, start.y + ((row1 == 0) ? 20 : -20) };

                Vector2 prev = start;
                for (int t = 1; t <= 12; t++) {
                    float f = t / 12.0f;
                    float u = 1.0f - f;
                    Vector2 pt = {
                        u * u * start.x + 2 * u * f * ctrl.x + f * f * end.x,
                        u * u * start.y + 2 * u * f * ctrl.y + f * f * end.y
                    };
                    DrawLineEx(prev, pt, 2.5f, lineCol);
                    prev = pt;
                }
            }
            else {
                DrawLineEx(Vector2{ (float)x1, (float)(y1 + circleRadius) },
                    Vector2{ (float)x2, (float)(y2 - circleRadius) }, 2.5f, lineCol);
            }
        }
    }

    for (int i = 0; i < 13; i++) {
        int cx = startX + i * spacing;
        char letter = 'A' + i;

        bool isHighlight = (i == highlightIn || i == highlightOut);
        bool isSelected = (i == selectedFirst);
        bool isPaired = (pb.mapping[i] != i);

        Color bg = isPaired ? pb.getPairColor(i) : Color{ 32, 32, 38, 255 };
        if (isSelected) bg = ORANGE;
        Color fg = isHighlight ? YELLOW : WHITE;

        DrawLetterCircle(cx, row1Y, circleRadius, letter, bg, fg, isHighlight);
    }

    for (int i = 0; i < 13; i++) {
        int cx = startX + i * spacing;
        int letterIdx = 13 + i;
        char letter = 'A' + letterIdx;

        bool isHighlight = (letterIdx == highlightIn || letterIdx == highlightOut);
        bool isSelected = (letterIdx == selectedFirst);
        bool isPaired = (pb.mapping[letterIdx] != letterIdx);

        Color bg = isPaired ? pb.getPairColor(letterIdx) : Color{ 32, 32, 38, 255 };
        if (isSelected) bg = ORANGE;
        Color fg = isHighlight ? YELLOW : WHITE;

        DrawLetterCircle(cx, row2Y, circleRadius, letter, bg, fg, isHighlight);
    }
}

void DrawSignalPath(const AppState& state, int x, int y, int width) {
    const EnigmaMachine& m = state.machine;
    const EncodingPath& p = state.currentPath;

    DrawTextF("signal path", x, y, 14, GRAY);
    y += 22;

    int boxHeight = 140;
    DrawRectangle(x, y, width, boxHeight, Color{ 22, 22, 28, 255 });
    DrawRectangleLines(x, y, width, boxHeight, Color{ 45, 45, 55, 255 });

    if (p.steps.empty()) {
        DrawTextF("type a letter to see encoding path", x + 60, y + 60, 13, Color{ 60,60,70,255 });
        return;
    }

    int currentStep = state.animating ? state.animationStep : (int)p.steps.size() - 1;
    int numRotors = m.rotors.size();

    int numComponents = numRotors + 2;
    int compSpacing = (width - 100) / numComponents;
    int startX = x + 70;

    const char* labels[10];
    labels[0] = "PB";
    for (int i = 0; i < numRotors; i++) {
        labels[i + 1] = ROTOR_NAMES[m.rotors[numRotors - 1 - i].rotorType];
    }
    labels[numRotors + 1] = "UKW";

    for (int i = 0; i < numComponents; i++) {
        int cx = startX + i * compSpacing;
        int labelW = MeasureTextF(labels[i], 10);
        DrawTextF(labels[i], cx - labelW / 2, y + boxHeight - 18, 10, Color{ 70,70,80,255 });
    }

    int forwardY = y + 38;
    int backwardY = y + 95;
    int inputOutputX = x + 25;
    int circleRadius = 12;

    int reflectorStepIdx = numRotors + 2;
    int totalSteps = (int)p.steps.size();

    auto getStepPos = [&](int stepIdx) -> Vector2 {
        if (stepIdx == 0) {
            return { (float)inputOutputX, (float)forwardY };
        }
        else if (stepIdx == totalSteps - 1) {
            return { (float)inputOutputX, (float)backwardY };
        }
        else if (stepIdx <= reflectorStepIdx) {
            int comp = stepIdx - 1;
            int cx = startX + comp * compSpacing;
            return { (float)cx, (float)forwardY };
        }
        else {
            int backwardIdx = stepIdx - reflectorStepIdx - 1;
            int comp = numRotors - backwardIdx;
            int cx = startX + comp * compSpacing;
            return { (float)cx, (float)backwardY };
        }
        };

    for (int i = 1; i <= currentStep && i < totalSteps; i++) {
        Vector2 pos = getStepPos(i);
        Vector2 prevPos = getStepPos(i - 1);

        Color lineCol = (i <= reflectorStepIdx) ? Color{ 70, 140, 70, 255 } : Color{ 140, 70, 70, 255 };

        if (std::fabs(pos.y - prevPos.y) < 1.0f) {
            float dir = (pos.x > prevPos.x) ? 1.0f : -1.0f;
            Vector2 lineStart = { prevPos.x + dir * circleRadius, prevPos.y };
            Vector2 lineEnd = { pos.x - dir * circleRadius, pos.y };
            DrawLineEx(lineStart, lineEnd, 2, lineCol);
        }
        else {
            Vector2 dir = { pos.x - prevPos.x, pos.y - prevPos.y };
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len > 0) {
                dir.x /= len;
                dir.y /= len;
            }
            Vector2 lineStart = { prevPos.x + dir.x * circleRadius, prevPos.y + dir.y * circleRadius };
            Vector2 lineEnd = { pos.x - dir.x * circleRadius, pos.y - dir.y * circleRadius };
            DrawLineEx(lineStart, lineEnd, 2, lineCol);
        }
    }

    for (int i = 0; i <= currentStep && i < totalSteps; i++) {
        Vector2 pos = getStepPos(i);

        char letter = 'A' + p.steps[i];
        bool isCurrent = (i == currentStep);
        Color bg = isCurrent ? YELLOW : Color{ 55, 55, 68, 255 };
        Color fg = isCurrent ? BLACK : WHITE;

        DrawLetterCircle((int)pos.x, (int)pos.y, circleRadius, letter, bg, fg, isCurrent);
    }

    DrawTextF("in", inputOutputX - 5, forwardY - 25, 12, Color{ 70,140,70,255 });
    DrawTextF("out", inputOutputX - 8, backwardY + 15, 12, Color{ 140,70,70,255 });
}

void DrawTextDisplay(const AppState& state, int x, int y, int width) {
    DrawTextF("message", x, y, 14, GRAY);
    y += 22;

    DrawRectangle(x, y, width, 70, Color{ 22, 22, 28, 255 });
    DrawRectangleLines(x, y, width, 70, Color{ 45, 45, 55, 255 });

    std::string formattedInput, formattedOutput;
    for (size_t i = 0; i < state.inputText.size(); i++) {
        formattedInput += state.inputText[i];
        if ((i + 1) % 5 == 0 && i + 1 < state.inputText.size()) formattedInput += ' ';
    }
    for (size_t i = 0; i < state.outputText.size(); i++) {
        formattedOutput += state.outputText[i];
        if ((i + 1) % 5 == 0 && i + 1 < state.outputText.size()) formattedOutput += ' ';
    }

    DrawTextF("in:", x + 10, y + 10, 12, Color{ 80,80,90,255 });
    DrawTextF(formattedInput.empty() ? "_" : formattedInput.c_str(), x + 38, y + 8, 15, WHITE);

    DrawTextF("out:", x + 10, y + 42, 12, Color{ 80,80,90,255 });
    DrawTextF(formattedOutput.empty() ? "_" : formattedOutput.c_str(), x + 38, y + 40, 15, YELLOW);
}

void DrawConfigPanel(AppState& state, int x, int y, int width) {
    EnigmaMachine& machine = state.machine;

    int openDropdown = state.activeDropdown;

    DrawTextF("configuration", x, y, 14, GRAY);
    y += 22;

    int panelHeight = 340;
    DrawRectangle(x, y, width, panelHeight, Color{ 20, 20, 26, 255 });
    DrawRectangleLines(x, y, width, panelHeight, Color{ 45, 45, 55, 255 });

    int innerX = x + 12;
    int innerY = y + 12;

    DrawTextF("machine", innerX, innerY, 12, Color{ 100,100,110,255 });
    innerY += 18;

    for (int i = 0; i < 3; i++) {
        Rectangle btn = { (float)(innerX + i * 72), (float)innerY, 68, 24 };
        bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
        bool selected = ((int)machine.type == i);

        Color bg = selected ? Color{ 60, 80, 60, 255 } : (hover ? Color{ 50, 50, 58, 255 } : Color{ 38, 38, 45, 255 });
        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, 1, selected ? Color{ 90,120,90,255 } : Color{ 55,55,65,255 });

        int tw = MeasureTextF(MACHINE_NAMES[i], 11);
        DrawTextF(MACHINE_NAMES[i], innerX + i * 72 + 34 - tw / 2, innerY + 6, 11, WHITE);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !selected) {
            machine.init((MachineType)i);
            state.inputText.clear();
            state.outputText.clear();
            state.currentPath.clear();
            state.rotorPositionHistory.clear();
            state.animating = false;
            state.activeDropdown = -1;
            state.plugboardFirst = -1;
        }
    }

    innerY += 36;

    DrawTextF("rotor order (left to right)", innerX, innerY, 12, Color{ 100,100,110,255 });
    innerY += 18;

    int rotorBtnW = 50;
    int rotorBtnY = innerY;

    for (size_t i = 0; i < machine.rotors.size(); i++) {
        int rx = innerX + i * (rotorBtnW + 4);
        Rectangle btn = { (float)rx, (float)innerY, (float)rotorBtnW, 22 };
        bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
        bool open = (state.activeDropdown == (int)(i + 1));

        DrawRectangleRec(btn, hover || open ? Color{ 50, 50, 58, 255 } : Color{ 38, 38, 45, 255 });
        DrawRectangleLinesEx(btn, 1, Color{ 55,55,65,255 });
        DrawTextF(ROTOR_NAMES[machine.rotors[i].rotorType], rx + 5, innerY + 4, 12, WHITE);
        DrawTextF(open ? "^" : "v", rx + rotorBtnW - 14, innerY + 5, 10, GRAY);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (open) {
                state.activeDropdown = -1;
            }
            else {
                state.activeDropdown = (i + 1);
            }
        }
    }

    innerY += 34;

    DrawTextF("positions", innerX, innerY, 12, Color{ 100,100,110,255 });
    innerY += 18;

    for (size_t i = 0; i < machine.rotors.size(); i++) {
        int rx = innerX + i * (rotorBtnW + 4);

        char posStr[2] = { (char)('A' + machine.rotors[i].position), '\0' };
        int tw = MeasureTextF(posStr, 18);
        DrawTextF(posStr, rx + rotorBtnW / 2 - tw / 2, innerY, 18, Color{ 120, 170, 100, 255 });

        Rectangle upBtn = { (float)rx, (float)(innerY + 22), 22, 18 };
        Rectangle downBtn = { (float)(rx + 26), (float)(innerY + 22), 22, 18 };

        bool upHover = CheckCollisionPointRec(GetMousePosition(), upBtn);
        bool downHover = CheckCollisionPointRec(GetMousePosition(), downBtn);

        DrawRectangleRec(upBtn, upHover ? Color{ 50, 65, 50, 255 } : Color{ 35, 45, 35, 255 });
        DrawRectangleRec(downBtn, downHover ? Color{ 65, 50, 50, 255 } : Color{ 45, 35, 35, 255 });

        int plusW = MeasureTextF("+", 12);
        int minusW = MeasureTextF("-", 12);
        DrawTextF("+", rx + 11 - plusW / 2, innerY + 24, 12, WHITE);
        DrawTextF("-", rx + 37 - minusW / 2, innerY + 24, 12, WHITE);

        if (upHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && openDropdown < 0) {
            machine.rotors[i].position = (machine.rotors[i].position + 1) % 26;
        }
        if (downHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && openDropdown < 0) {
            machine.rotors[i].position = (machine.rotors[i].position + 25) % 26;
        }
    }

    innerY += 48;

    DrawTextF("ring settings", innerX, innerY, 12, Color{ 100,100,110,255 });
    innerY += 18;

    for (size_t i = 0; i < machine.rotors.size(); i++) {
        int rx = innerX + i * (rotorBtnW + 4);

        char ringStr[2] = { (char)('A' + machine.rotors[i].ringSetting), '\0' };
        int tw = MeasureTextF(ringStr, 18);
        DrawTextF(ringStr, rx + rotorBtnW / 2 - tw / 2, innerY, 18, Color{ 100, 140, 160, 255 });

        Rectangle upBtn = { (float)rx, (float)(innerY + 22), 22, 18 };
        Rectangle downBtn = { (float)(rx + 26), (float)(innerY + 22), 22, 18 };

        bool upHover = CheckCollisionPointRec(GetMousePosition(), upBtn);
        bool downHover = CheckCollisionPointRec(GetMousePosition(), downBtn);

        DrawRectangleRec(upBtn, upHover ? Color{ 50, 65, 50, 255 } : Color{ 35, 45, 35, 255 });
        DrawRectangleRec(downBtn, downHover ? Color{ 65, 50, 50, 255 } : Color{ 45, 35, 35, 255 });

        int plusW = MeasureTextF("+", 12);
        int minusW = MeasureTextF("-", 12);
        DrawTextF("+", rx + 11 - plusW / 2, innerY + 24, 12, WHITE);
        DrawTextF("-", rx + 37 - minusW / 2, innerY + 24, 12, WHITE);

        if (upHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && openDropdown < 0) {
            machine.rotors[i].ringSetting = (machine.rotors[i].ringSetting + 1) % 26;
        }
        if (downHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && openDropdown < 0) {
            machine.rotors[i].ringSetting = (machine.rotors[i].ringSetting + 25) % 26;
        }
    }

    innerY += 48;

    DrawTextF("reflector", innerX, innerY, 12, Color{ 100,100,110,255 });
    innerY += 18;

    int numRefl = (machine.type == MACHINE_M4) ? 2 : 3;
    int startRefl = (machine.type == MACHINE_M4) ? 3 : 0;

    for (int i = 0; i < numRefl; i++) {
        Rectangle btn = { (float)(innerX + i * 78), (float)innerY, 74, 22 };
        bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
        bool selected = (machine.reflector.type == startRefl + i);

        Color bg = selected ? Color{ 60, 60, 80, 255 } : (hover ? Color{ 50, 50, 58, 255 } : Color{ 38, 38, 45, 255 });
        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, 1, selected ? Color{ 90,90,120,255 } : Color{ 55,55,65,255 });
        DrawTextF(REFLECTOR_NAMES[startRefl + i], innerX + i * 78 + 5, innerY + 5, 11, WHITE);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && openDropdown < 0) {
            machine.reflector.init(startRefl + i);
        }
    }

    innerY += 32;

    DrawTextF("speed", innerX, innerY, 12, Color{ 100,100,110,255 });
    Rectangle sliderBg = { (float)(innerX + 50), (float)(innerY - 2), 120, 18 };
    DrawRectangleRec(sliderBg, Color{ 28, 28, 35, 255 });

    float sliderPos = (state.animationSpeed - 0.1f) / 1.9f;
    Rectangle handle = { (float)(innerX + 50 + sliderPos * 108), (float)(innerY - 2), 12, 18 };
    DrawRectangleRec(handle, LIGHTGRAY);

    if (CheckCollisionPointRec(GetMousePosition(), sliderBg) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        float newPos = (GetMousePosition().x - innerX - 50) / 120.0f;
        state.animationSpeed = 0.1f + Clamp(newPos, 0.0f, 1.0f) * 1.9f;
    }

    char speedStr[16];
    snprintf(speedStr, sizeof(speedStr), "%.1fx", state.animationSpeed);
    DrawTextF(speedStr, innerX + 180, innerY, 12, WHITE);

    innerY += 28;

    Rectangle clearBtn = { (float)innerX, (float)innerY, 100, 24 };
    Rectangle resetBtn = { (float)(innerX + 108), (float)innerY, 100, 24 };

    bool clearHover = CheckCollisionPointRec(GetMousePosition(), clearBtn);
    bool resetHover = CheckCollisionPointRec(GetMousePosition(), resetBtn);

    DrawRectangleRec(clearBtn, clearHover ? Color{ 80, 45, 45, 255 } : Color{ 55, 35, 35, 255 });
    DrawRectangleRec(resetBtn, resetHover ? Color{ 45, 45, 80, 255 } : Color{ 35, 35, 55, 255 });
    DrawTextF("clear plugs", innerX + 14, innerY + 6, 11, WHITE);
    DrawTextF("reset all", innerX + 130, innerY + 6, 11, WHITE);

    if (clearHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        machine.plugboard.init();
        state.plugboardFirst = -1;
    }
    if (resetHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (auto& r : machine.rotors) r.position = 0;
        state.inputText.clear();
        state.outputText.clear();
        state.currentPath.clear();
        state.rotorPositionHistory.clear();
        state.animating = false;
    }

    for (size_t i = 0; i < machine.rotors.size(); i++) {
        bool open = (state.activeDropdown == (int)(i + 1));
        if (!open) continue;

        int rx = innerX + i * (rotorBtnW + 4);

        int numOpts, startOpt;
        if (machine.type == MACHINE_M4 && i == 0) { numOpts = 2; startOpt = 8; }
        else if (machine.type == MACHINE_M3 || machine.type == MACHINE_M4) { numOpts = 8; startOpt = 0; }
        else { numOpts = 5; startOpt = 0; }

        Rectangle dropBg = { (float)rx - 1, (float)(rotorBtnY + 23), (float)rotorBtnW + 2, (float)(numOpts * 20 + 2) };
        DrawRectangleRec(dropBg, Color{ 35, 35, 45, 255 });
        DrawRectangleLinesEx(dropBg, 1, Color{ 60,60,70,255 });

        for (int j = 0; j < numOpts; j++) {
            int candidateType = startOpt + j;
            bool isDuplicate = false;
            if (!(machine.type == MACHINE_M4 && i == 0)) {
                for (size_t k = 0; k < machine.rotors.size(); k++) {
                    if (k != i && machine.rotors[k].rotorType == candidateType) {
                        isDuplicate = true;
                        break;
                    }
                }
            }

            Rectangle opt = { (float)rx, (float)(rotorBtnY + 24 + j * 20), (float)rotorBtnW, 20 };
            bool optHover = CheckCollisionPointRec(GetMousePosition(), opt);
            Color optBg = isDuplicate ? Color{ 30, 30, 36, 255 } : (optHover ? Color{ 60, 60, 70, 255 } : Color{ 42, 42, 52, 255 });
            Color optFg = isDuplicate ? Color{ 95, 95, 105, 255 } : WHITE;
            DrawRectangleRec(opt, optBg);
            DrawTextF(ROTOR_NAMES[candidateType], rx + 5, rotorBtnY + 26 + j * 20, 11, optFg);

            if (!isDuplicate && optHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int oldPosition = machine.rotors[i].position;
                int oldRingSetting = machine.rotors[i].ringSetting;
                machine.rotors[i].init(candidateType);
                machine.rotors[i].position = oldPosition;
                machine.rotors[i].ringSetting = oldRingSetting;
                state.activeDropdown = -1;
            }
        }

        if (openDropdown == (int)(i + 1) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(GetMousePosition(), dropBg)) {
            state.activeDropdown = -1;
        }
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "enigma machine");
    SetTargetFPS(60);

    appFont = LoadFontEx("Roboto-Medium.ttf", 32, nullptr, 250);
    if (appFont.texture.id == 0) appFont = GetFontDefault();
    SetTextureFilter(appFont.texture, TEXTURE_FILTER_BILINEAR);

    AppState state;
    state.machine.init(MACHINE_ENIGMA_I);
    state.animationSpeed = 1.0f;
    state.plugboardFirst = -1;
    state.activeDropdown = -1;

    while (!WindowShouldClose()) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 'a' && key <= 'z') key = key - 'a' + 'A';
            if (key >= 'A' && key <= 'Z') {
                int inputIdx = key - 'A';
                std::vector<int> priorPositions;
                priorPositions.reserve(state.machine.rotors.size());
                for (const auto& r : state.machine.rotors) {
                    priorPositions.push_back(r.position);
                }
                state.rotorPositionHistory.push_back(priorPositions);

                int output = state.machine.encode(inputIdx, state.currentPath);
                state.inputText += (char)key;
                state.outputText += (char)('A' + output);
                state.animating = true;
                state.animationTime = 0;
                state.animationStep = 0;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !state.inputText.empty()) {
            state.inputText.pop_back();
            state.outputText.pop_back();
            state.currentPath.clear();
            state.animating = false;
            if (!state.rotorPositionHistory.empty()) {
                const std::vector<int>& priorPositions = state.rotorPositionHistory.back();
                for (size_t i = 0; i < state.machine.rotors.size() && i < priorPositions.size(); i++) {
                    state.machine.rotors[i].position = priorPositions[i];
                }
                state.rotorPositionHistory.pop_back();
            }
        }

        // plugboard interaction - coordinates must match DrawPlugboard
        int pbX = 20;
        int pbY = 523;
        int pbStartX = pbX + 18;
        int pbSpacing = 33;
        int pbRow1Y = pbY + 22 + 26;  // y + 22 (label offset) + 26 (first row center)
        int pbRow2Y = pbY + 22 + 72;  // y + 22 (label offset) + 72 (second row center)

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int i = 0; i < 26; i++) {
                int row = i / 13;
                int col = i % 13;
                int cx = pbStartX + col * pbSpacing;
                int cy = (row == 0) ? pbRow1Y : pbRow2Y;

                if (CheckCollisionPointCircle(GetMousePosition(), Vector2{ (float)cx, (float)cy }, 13)) {
                    if (state.plugboardFirst == -1) {
                        state.plugboardFirst = i;
                    }
                    else if (state.plugboardFirst == i) {
                        state.machine.plugboard.clearLetter(i);
                        state.plugboardFirst = -1;
                    }
                    else {
                        state.machine.plugboard.setPair(state.plugboardFirst, i);
                        state.plugboardFirst = -1;
                    }
                    break;
                }
            }
        }

        if (state.animating) {
            state.animationTime += GetFrameTime() * state.animationSpeed * 4.0f;
            state.animationStep = (int)state.animationTime;
            if (state.animationStep >= (int)state.currentPath.steps.size()) {
                state.animating = false;
            }
        }

        BeginDrawing();
        ClearBackground(Color{ 16, 16, 22, 255 });

        DrawTextF("enigma machine", 20, 12, 22, Color{ 180, 150, 90, 255 });

        // left side
        int pressedKey = state.animating ? state.currentPath.inputLetter : -1;
        int litLamp = state.animating ? -1 : state.currentPath.outputLetter;

        DrawLampboard(20, 45, litLamp);
        DrawKeyboard(20, 175, pressedKey);
        DrawRotorWindows(state.machine, 20, 320);
        DrawTextDisplay(state, 20, 420, 450);
        DrawPlugboard(state.machine.plugboard, 20, 523,
            state.animating ? state.currentPath.steps[0] : -1,
            (state.animating && state.animationStep >= 1) ? state.currentPath.steps[1] : -1,
            state.plugboardFirst);

        // right side
        DrawSignalPath(state, 500, 45, 680);
        DrawConfigPanel(state, 500, 230, 680);

        DrawTextF("type a-z to encode | click plugboard letters to connect | backspace to delete", 20, SCREEN_HEIGHT - 22, 11, Color{ 60,60,70,255 });

        EndDrawing();
    }

    UnloadFont(appFont);
    CloseWindow();
    return 0;
}
