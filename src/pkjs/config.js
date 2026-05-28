module.exports = [
  {
    "type": "heading",
    "defaultValue": "Majora's Mask Watchface"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Clock",
        "size": 2
      },
      {
        "type": "toggle",
        "messageKey": "KEY_24H",
        "label": "24-Hour Time",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Second Hand",
        "size": 2
      },
      {
        "type": "toggle",
        "messageKey": "KEY_ANIM_ENABLED",
        "label": "Smooth Animation",
        "description": "Sweeps the icon across the arc. Disable to save battery.",
        "defaultValue": true
      },
      {
        "type": "select",
        "messageKey": "KEY_ANIM_DURATION",
        "label": "Sweep Duration",
        "description": "How long the sweep lasts each second. Shorter = less battery drain.",
        "defaultValue": 900,
        "options": [
          { "label": "Tick (no sweep)", "value": 0 },
          { "label": "Short (500 ms)",  "value": 500 },
          { "label": "Normal (700 ms)", "value": 700 },
          { "label": "Long (900 ms)",   "value": 900 },
          { "label": "Continuous",      "value": 1000 }
        ]
      },
      {
        "type": "select",
        "messageKey": "KEY_TIMEOUT_S",
        "label": "Hide After",
        "description": "Seconds before the second hand disappears. Matches your Pebble's backlight timeout for best results.",
        "defaultValue": 10,
        "options": [
          { "label": "Always visible", "value": 0 },
          { "label": "5 seconds",      "value": 5 },
          { "label": "10 seconds",     "value": 10 },
          { "label": "15 seconds",     "value": 15 },
          { "label": "30 seconds",     "value": 30 }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Health",
        "size": 2
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SHOW_HR",
        "label": "Show Heart Rate",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SHOW_STEPS",
        "label": "Show Step Count",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
