# Telecontrol SCADA Client

## Discovery

http://telecontrol.ru/discovery.json

## Telemetry

Add to the `discovery.json` when possible:

```
  "telemetry": "https://d26i7akorx31n9.cloudfront.net/telemetry",
```

## Updates

### Use cases

* Check for updates once per hour.
* Once an update is detected, register a local event.
* An option to stop all update checks.
* A command to download the update.

### Schema

```json
{
  "scada": {
    "versions": {
      "2.3.8": {
        "description": "Many updates",
        "installer": "https://telecontrol-public.s3-us-west-2.amazonaws.com/telecontrol-scada/telecontrol-scada-2.3.8.msi",
      }
    }
  }
}
```