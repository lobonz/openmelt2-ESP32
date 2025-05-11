# TinyLinePlot

A minimal, lightweight JavaScript line plotting library designed for IoT devices and real-time data visualization. TinyLinePlot provides essential charting functionality in a compact package (~2.5KB minified).

![TinyLinePlot Sample](./TinyLinePlotExample.png)

## Features

- Minimal dependencies - pure JavaScript with no external libraries required
- Lightweight (~2.5KB minified) - ideal for IoT and embedded applications
- Multiple datasets with automatic coloring
- Real-time data streaming with auto-scrolling
- Historical data retention with navigation controls
- Customizable appearance, including colors, axes, grid, and more
- Responsive design
- Simple API for quick implementation

## Installation

### Direct Include

Simply download `TinyLinePlot.js` and include it in your HTML:

```html
<script src="TinyLinePlot.js"></script>
```

## Basic Usage

### 1. Create a canvas element

```html
<canvas id="myPlot"></canvas>
```

### 2. Initialize the plot

```javascript
const plot = new TinyLinePlot('myPlot', {
  width: 600,
  height: 300,
  xLabel: 'Time (s)',
  yLabel: 'Temperature (°C)',
  title: 'Sensor Reading'
});
```

### 3. Add data and draw

```javascript
// Create a dataset
const data = [
  { x: 0, y: 10 },
  { x: 1, y: 15 },
  { x: 2, y: 13 },
  { x: 3, y: 17 }
];

// Add dataset and draw
plot.addDataset(data, 'Temperature').draw();
```

## Streaming Data Example

```html
<div>
  <button id="startBtn">Start Streaming</button>
  <button id="stopBtn">Stop Streaming</button>
  <button id="clearBtn">Clear Data</button>
</div>
<canvas id="streamPlot"></canvas>

<script>
  // Initialize plot with streaming and data retention enabled
  const plot = new TinyLinePlot('streamPlot', {
    width: 600,
    height: 300,
    xLabel: 'Time (s)',
    yLabel: 'Value',
    title: 'Live Data Stream',
    streaming: true,
    retainData: true,
    viewWindowSize: 50
  });
  
  // Add empty dataset
  plot.addDataset([], 'Sensor').draw();
  
  let interval;
  let time = 0;
  
  // Stream data function
  function addData() {
    plot.addPoint(0, { 
      x: time, 
      y: Math.sin(time * 0.1) * 10 + Math.random() * 2 
    }).draw();
    time += 0.5;
  }
  
  // Start streaming
  document.getElementById('startBtn').addEventListener('click', function() {
    if (!interval) {
      interval = setInterval(addData, 500);
    }
  });
  
  // Stop streaming
  document.getElementById('stopBtn').addEventListener('click', function() {
    if (interval) {
      clearInterval(interval);
      interval = null;
    }
  });
  
  // Clear data
  document.getElementById('clearBtn').addEventListener('click', function() {
    plot.clear().addDataset([], 'Sensor').draw();
    time = 0;
  });
</script>
```

## Advanced Usage: Historical Data Navigation

```html
<div>
  <button id="startBtn">Start</button>
  <button id="stopBtn">Stop</button>
  <button id="clearBtn">Clear</button>
  <button id="autoScrollBtn">Auto-scroll</button>
</div>
<div>
  <button id="startScroll"><<</button>
  <button id="scrollLeft"><</button>
  <input type="range" id="scrollSlider">
  <button id="scrollRight">></button>
  <button id="endScroll">>></button>
</div>
<canvas id="historyPlot"></canvas>

<script>
  // Initialize with data retention enabled
  const plot = new TinyLinePlot('historyPlot', {
    width: 700,
    height: 350,
    retainData: true,
    streaming: true,
    viewWindowSize: 50
  });
  
  plot.addDataset([], 'Data').draw();
  
  // ... Streaming implementation (similar to previous example) ...
  
  // Navigation controls
  document.getElementById('startScroll').addEventListener('click', function() {
    plot.scrollToStart().draw();
  });
  
  document.getElementById('scrollLeft').addEventListener('click', function() {
    plot.scroll(-10).draw(); // Move back 10 points
  });
  
  document.getElementById('scrollRight').addEventListener('click', function() {
    plot.scroll(10).draw(); // Move forward 10 points
  });
  
  document.getElementById('endScroll').addEventListener('click', function() {
    plot.scrollToEnd().draw(); // Go to newest data
  });
  
  document.getElementById('autoScrollBtn').addEventListener('click', function() {
    plot.toggleAutoScroll().draw(); // Toggle auto-scrolling
  });
  
  // Slider implementation
  const slider = document.getElementById('scrollSlider');
  slider.addEventListener('input', function() {
    plot.scrollTo(parseInt(this.value)).draw();
  });
  
  // Update slider based on data length
  function updateControls() {
    const dataLength = plot.getDataLength();
    const windowSize = plot.viewWindow.size;
    
    slider.max = Math.max(0, dataLength - windowSize);
    slider.value = plot.viewWindow.start;
  }
</script>
```

## API Reference

### Constructor

```javascript
new TinyLinePlot(elementId, options)
```

- `elementId`: The ID of the canvas element
- `options`: Object containing plot configuration (see Options below)

### Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `width` | Number | 300 | Canvas width in pixels |
| `height` | Number | 200 | Canvas height in pixels |
| `padding` | Number | 30 | Padding around the plot area |
| `lineColors` | Array | ['#3366CC', '#DC3912', ...] | Array of colors for datasets |
| `backgroundColor` | String | 'white' | Background color |
| `axisColor` | String | '#333333' | Color of axes |
| `labelColor` | String | '#666666' | Color of labels |
| `xLabel` | String | '' | X-axis label |
| `yLabel` | String | '' | Y-axis label |
| `title` | String | '' | Plot title |
| `showGrid` | Boolean | false | Show grid lines |
| `gridColor` | String | '#EEEEEE' | Grid line color |
| `lineWidth` | Number | 2 | Line width for datasets |
| `dotSize` | Number | 0 | Size of dots at data points (0 = no dots) |
| `maxPoints` | Number | 100 | Maximum points when not retaining data |
| `streaming` | Boolean | false | Enable streaming data mode |
| `retainData` | Boolean | false | Keep all historical data |
| `viewWindowSize` | Number | 50 | Number of points visible at once |
| `autoScrollWithNewData` | Boolean | true | Auto-scroll to show new data |

### Methods

#### Data Management

- **`addDataset(data, label)`**: Adds a new dataset
  - `data`: Array of {x, y} points
  - `label`: Optional label for the dataset
  - Returns: The plot instance (for chaining)

- **`addPoint(datasetIndex, point)`**: Adds a point to an existing dataset
  - `datasetIndex`: Index of the dataset
  - `point`: {x, y} point to add
  - Returns: The plot instance

- **`clear()`**: Clears all datasets
  - Returns: The plot instance

- **`clearDataset(datasetIndex)`**: Clears specific dataset
  - `datasetIndex`: Index of the dataset to clear
  - Returns: The plot instance

#### Navigation

- **`scroll(steps)`**: Scrolls the view window by steps
  - `steps`: Number of steps to scroll (positive or negative)
  - Returns: The plot instance

- **`scrollTo(position)`**: Jumps to a specific position
  - `position`: Starting position to scroll to
  - Returns: The plot instance

- **`scrollToStart()`**: Scrolls to the beginning of data
  - Returns: The plot instance

- **`scrollToEnd()`**: Scrolls to the most recent data
  - Returns: The plot instance

#### Auto-Scroll Control

- **`enableAutoScroll()`**: Enables auto-scrolling with new data
  - Returns: The plot instance

- **`disableAutoScroll()`**: Disables auto-scrolling with new data
  - Returns: The plot instance

- **`toggleAutoScroll()`**: Toggles auto-scrolling state
  - Returns: The plot instance

#### Drawing

- **`draw()`**: Renders the plot
  - Returns: The plot instance

#### Utility

- **`getDataLength()`**: Gets the maximum number of points in any dataset
  - Returns: Number of points

## Browser Compatibility

TinyLinePlot works in all modern browsers with HTML5 Canvas support:

- Chrome
- Firefox
- Safari
- Edge

## License

MIT License 
