---
name: public-transportation
description: Instructions for configuring and using the Public Transportation application integration.
---

# Public Transportation Application Integration

The Public Transportation integration allows for quick opening of a local transport monitoring application via a hardware macro or daemon command.

## App Management

```bash
# Using generic app commands
d appStatus --app pt                  # Check status
d startApp --app pt --mode prod       # Start production
d stopApp --app pt --mode all         # Stop all modes
d restartApp --app pt --mode dev      # Restart dev

# Install dependencies
d installAppDeps --app pt --mode prod

# Quick open (legacy command)
d publicTransportationOpenApp         # Opens in Chrome
```

## Port Configuration

| Mode | Port | Key |
|------|------|-----|
| Production | 3002 | `pt-prod` |
| Development | 3003 | `pt-dev` |

To view or update ports:

```bash
d getPort --key pt-prod               # Query current port
d setPort --key pt-prod --value 3002  # Set port
d listPorts                           # List all ports
```

## Commands

- `d publicTransportationOpenApp`: Opens the PT app in Chrome using the registered port
- `d publicTransportationStartProxy`: (Optional) Start the SSH proxy required for the app's data source

## File Locations

| Mode | Path |
|------|------|
| Development | `/opt/dev/publicTransportation` |
| Production | `/opt/prod/publicTransportation` |

## Troubleshooting

- **Chrome doesn't open**: Ensure `google-chrome` is in your PATH
- **Port issue**: Verify the port is correctly set: `d getPort --key pt-prod`
- **Macro not firing**: Check daemon logs for "Triggering G3" or "Triggering Public Transportation macro"
- **Dependencies missing**: Run `d installAppDeps --app pt --mode prod`
