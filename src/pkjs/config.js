module.exports = [
  {
    type: 'heading',
    defaultValue: 'Rainy Windowpane'
  },
  {
    type: 'text',
    defaultValue:
      'Pixel rain on foggy glass. The time is wiped into the condensation and slowly fades until the next minute.'
  },

  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Time'
      },
      {
        type: 'select',
        messageKey: 'TimeFormat',
        defaultValue: '0',
        label: 'Hour format',
        options: [
          { label: 'System default', value: '0' },
          { label: '12-hour', value: '1' },
          { label: '24-hour', value: '2' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'LeadingZero',
        label: 'Leading zero on hours',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'ShowDate',
        label: 'Show date',
        defaultValue: false
      }
    ]
  },

  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Window'
      },
      {
        type: 'select',
        messageKey: 'RainIntensity',
        defaultValue: '2',
        label: 'Rain',
        options: [
          { label: 'Off', value: '0' },
          { label: 'Light', value: '1' },
          { label: 'Medium', value: '2' },
          { label: 'Heavy', value: '3' }
        ]
      },
      {
        type: 'select',
        messageKey: 'FadeSpeed',
        defaultValue: '1',
        label: 'Condensation fade',
        options: [
          { label: 'Slow', value: '0' },
          { label: 'Normal', value: '1' },
          { label: 'Fast', value: '2' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'ShowSeason',
        label: 'Show season caption',
        defaultValue: false
      }
    ]
  },

  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Outside'
      },
      {
        type: 'text',
        defaultValue:
          'Auto uses your watch clock and calendar. Override to preview a season or time of day.'
      },
      {
        type: 'select',
        messageKey: 'SeasonMode',
        defaultValue: '0',
        label: 'Season',
        options: [
          { label: 'Auto (from date)', value: '0' },
          { label: 'Spring', value: '1' },
          { label: 'Summer', value: '2' },
          { label: 'Autumn', value: '3' },
          { label: 'Winter', value: '4' }
        ]
      },
      {
        type: 'select',
        messageKey: 'TimeOfDayMode',
        defaultValue: '0',
        label: 'Time of day',
        options: [
          { label: 'Auto (from clock)', value: '0' },
          { label: 'Dawn', value: '1' },
          { label: 'Day', value: '2' },
          { label: 'Dusk', value: '3' },
          { label: 'Night', value: '4' }
        ]
      }
    ]
  },

  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
