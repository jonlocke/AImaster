# AImaster

**Version**: 1.0  
**Last Updated**: 2025-08-01  

## Preface

**AImaster** is a Windows-based serial shell interface that reads 3-character commands via a serial COM port. It supports alias mapping to real shell commands, displays output through a serial console, and offers special AI-powered commands like `ASK` (using an Ollama LLM server) and `HLP` (to view help documentation). Its purpose is to create a minimal interactive environment for embedded systems, test rigs, or automation setups with basic AI capabilities.

---

## Features

- Serial command input and output
- Command aliasing via `cmds.csv`
- Customizable prompt: `8->`
- Built-in `ASK` command to query an LLM via Ollama
- `HLP` command to print help documentation
- Configurable via `config.json`
- Outputs AI responses to `log.txt` with timestamps
- CR (`\r`) converted from newline for proper Acorn Electron-style display
- Optional CLI or config-file driven initialization

---

## How It Works

- The program connects to the specified COM port at 2400 baud.
- It waits for a 3-character command from the serial terminal.
- It looks up the command in `cmds.csv`.
- If found:
  - Executes the mapped shell command and returns output
  - OR invokes special handlers like `ASK` and `HLP`
- If not found:
  - Displays `Unknown command: XXX`
- After every command, prompt `8->` is shown.

---

## Configuration

### `config.json`

Provides default runtime settings if not supplied via command-line arguments.

```json
{
  "com_port": "COM7",
  "send_delay_ms": 10,
  "initial_message": "Welcome to AImaster!",
  "http_timeout_ms": 60000
}
```

### Command-Line Alternative

```bash
SerialSender.exe COM7 10 "Welcome to AImaster!"
```

---

## Commands and Aliases

### `cmds.csv`

Each line maps a 3-letter alias to a shell command or special function:

```
DIR:DIR
WHO:WHOAMI
ASK:ASK=MODEL=llama3
HLP:HLP
```

- `ASK` supports optional model override.
- `HLP` reads from `help.txt`.

---

## Output

- Standard command output is shown via serial with delays between characters.
- AI response from Ollama is decoded and formatted.
- Results are logged to `log.txt` with a timestamp.

---

## Sample Files

### `help.txt`

```txt
Welcome to AImaster!
Available commands:
DIR - List files
WHO - Show user
ASK - Ask a question to AI
```

---

### `log.txt`

```
Wed Aug 1 19:53:02 2025
Hello! I am your assistant.

```

---

## Screenshots

> *(Add screenshots of serial terminal sessions, prompt usage, and example `ASK` interaction)*

---

## Troubleshooting

- **"Failed to open COM port"**: Ensure device is connected and `config.json` or CLI specifies the right port.
- **"Ollama response missing"**: Verify Ollama server is running at `127.0.0.1:11434`.
- **"Unknown command"**: Ensure alias exists in `cmds.csv`.

---

## License

MIT License — use freely with attribution.

---

## Contact

For bugs or enhancements, open an issue or contact the author.
