/**
 * TinyLinePlot.js - A minimal line plotting library for IoT devices
 * Total size: ~2.5KB minified
 */

class TinyLinePlot {
  constructor(elementId, options = {}) {
    // Get canvas element
    this.canvas = document.getElementById(elementId);
    this.ctx = this.canvas.getContext('2d');
    
    // Default options
    this.options = {
      width: options.width || this.canvas.width || 300,
      height: options.height || this.canvas.height || 200,
      padding: options.padding || 30,
      lineColors: options.lineColors || ['#3366CC', '#DC3912', '#FF9900', '#109618', '#990099'],
      backgroundColor: options.backgroundColor || 'white',
      axisColor: options.axisColor || '#333333',
      labelColor: options.labelColor || '#666666',
      xLabel: options.xLabel || '',
      yLabel: options.yLabel || '',
      title: options.title || '',
      showGrid: options.showGrid !== undefined ? options.showGrid : false,
      gridColor: options.gridColor || '#EEEEEE',
      lineWidth: options.lineWidth || 2,
      dotSize: options.dotSize || 0,
      maxPoints: options.maxPoints || 100,
      streaming: options.streaming !== undefined ? options.streaming : false,
      retainData: options.retainData !== undefined ? options.retainData : false,
      viewWindowSize: options.viewWindowSize || 50,
      autoScrollWithNewData: options.autoScrollWithNewData !== undefined ? options.autoScrollWithNewData : true
    };
    
    // Set canvas dimensions
    this.canvas.width = this.options.width;
    this.canvas.height = this.options.height;
    
    // Calculate plotting area
    this.plotArea = {
      x: this.options.padding * 1.5,
      y: this.options.padding,
      width: this.options.width - this.options.padding * 2.5,
      height: this.options.height - this.options.padding * 2
    };
    
    // Initialize data
    this.datasets = [];
    
    // Initialize view window (for scrolling through data)
    this.viewWindow = {
      start: 0,
      size: this.options.viewWindowSize
    };
    
    // Track if we should auto-scroll with new data
    this.autoScroll = this.options.autoScrollWithNewData;
  }
  
  addDataset(data, label = '') {
    const colorIndex = this.datasets.length % this.options.lineColors.length;
    this.datasets.push({
      data: data,
      label: label,
      color: this.options.lineColors[colorIndex]
    });
    return this;
  }
  
  addPoint(datasetIndex, point) {
    if (datasetIndex < 0 || datasetIndex >= this.datasets.length) {
      console.error('Dataset index out of bounds');
      return this;
    }
    
    const dataset = this.datasets[datasetIndex];
    dataset.data.push(point);
    
    // If streaming is enabled and we're not retaining data, remove oldest points
    if (this.options.streaming && !this.options.retainData && dataset.data.length > this.options.maxPoints) {
      dataset.data = dataset.data.slice(-this.options.maxPoints);
    }
    
    // If retaining data and streaming, adjust view window to show most recent data
    // but only if autoScroll is enabled
    if (this.options.streaming && this.options.retainData && this.autoScroll) {
      const maxIdx = dataset.data.length - 1;
      if (maxIdx >= this.options.viewWindowSize) {
        this.viewWindow.start = maxIdx - this.options.viewWindowSize + 1;
      }
    }
    
    return this;
  }
  
  clear() {
    this.datasets = [];
    this.ctx.clearRect(0, 0, this.options.width, this.options.height);
    this.viewWindow.start = 0;
    this.autoScroll = this.options.autoScrollWithNewData;
    return this;
  }
  
  // Clear a specific dataset
  clearDataset(datasetIndex) {
    if (datasetIndex >= 0 && datasetIndex < this.datasets.length) {
      this.datasets[datasetIndex].data = [];
    }
    this.viewWindow.start = 0;
    this.autoScroll = this.options.autoScrollWithNewData;
    return this;
  }
  
  // Enable auto-scrolling with new data
  enableAutoScroll() {
    this.autoScroll = true;
    return this;
  }
  
  // Disable auto-scrolling with new data
  disableAutoScroll() {
    this.autoScroll = false;
    return this;
  }
  
  // Toggle auto-scrolling with new data
  toggleAutoScroll() {
    this.autoScroll = !this.autoScroll;
    return this;
  }
  
  // Scroll view window
  scroll(steps) {
    // Find maximum dataset length
    let maxDataLength = 0;
    this.datasets.forEach(dataset => {
      maxDataLength = Math.max(maxDataLength, dataset.data.length);
    });
    
    // Calculate new start position
    const newStart = this.viewWindow.start + steps;
    
    // Ensure we don't scroll past the data
    if (newStart >= 0 && newStart <= Math.max(0, maxDataLength - this.viewWindow.size)) {
      this.viewWindow.start = newStart;
      // Disable auto-scroll when manually scrolling backward
      if (steps < 0) {
        this.autoScroll = false;
      }
    }
    
    return this;
  }
  
  // Jump to specific position
  scrollTo(position) {
    // Find maximum dataset length
    let maxDataLength = 0;
    this.datasets.forEach(dataset => {
      maxDataLength = Math.max(maxDataLength, dataset.data.length);
    });
    
    // Check if position is at the latest data
    const isAtEnd = position >= maxDataLength - this.viewWindow.size;
    
    // Validate position
    if (position >= 0 && position <= Math.max(0, maxDataLength - this.viewWindow.size)) {
      this.viewWindow.start = position;
      
      // If not scrolling to the end, disable auto-scroll
      if (!isAtEnd) {
        this.autoScroll = false;
      }
    }
    
    return this;
  }
  
  // Scroll to the beginning of the data
  scrollToStart() {
    this.viewWindow.start = 0;
    this.autoScroll = false;
    return this;
  }
  
  // Scroll to the end of the data (most recent)
  scrollToEnd() {
    let maxDataLength = 0;
    this.datasets.forEach(dataset => {
      maxDataLength = Math.max(maxDataLength, dataset.data.length);
    });
    
    this.viewWindow.start = Math.max(0, maxDataLength - this.viewWindow.size);
    this.autoScroll = true;
    return this;
  }
  
  findMinMax() {
    if (this.datasets.length === 0) return { minX: 0, maxX: 10, minY: 0, maxY: 10 };
    
    let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
    
    this.datasets.forEach(dataset => {
      // Only consider points within the current view window
      const start = this.options.retainData ? this.viewWindow.start : 0;
      const end = this.options.retainData ? 
        Math.min(start + this.viewWindow.size, dataset.data.length) : 
        dataset.data.length;
      
      for (let i = start; i < end; i++) {
        const point = dataset.data[i];
        if (point) {
          minX = Math.min(minX, point.x);
          maxX = Math.max(maxX, point.x);
          minY = Math.min(minY, point.y);
          maxY = Math.max(maxY, point.y);
        }
      }
    });
    
    // If no points in view, return default
    if (minX === Infinity) {
      return { minX: 0, maxX: 10, minY: 0, maxY: 10 };
    }
    
    // Add a small padding to the max/min values
    const rangeX = maxX - minX;
    const rangeY = maxY - minY;
    
    return {
      minX: minX - rangeX * 0.05,
      maxX: maxX + rangeX * 0.05,
      minY: minY - rangeY * 0.05,
      maxY: maxY + rangeY * 0.05
    };
  }
  
  scalePoint(point, minMax) {
    return {
      x: this.plotArea.x + (point.x - minMax.minX) / (minMax.maxX - minMax.minX) * this.plotArea.width,
      y: this.plotArea.y + this.plotArea.height - (point.y - minMax.minY) / (minMax.maxY - minMax.minY) * this.plotArea.height
    };
  }
  
  drawAxes(minMax) {
    const ctx = this.ctx;
    
    // Draw axes
    ctx.beginPath();
    ctx.strokeStyle = this.options.axisColor;
    ctx.lineWidth = 1;
    
    // X-axis
    ctx.moveTo(this.plotArea.x, this.plotArea.y + this.plotArea.height);
    ctx.lineTo(this.plotArea.x + this.plotArea.width, this.plotArea.y + this.plotArea.height);
    
    // Y-axis
    ctx.moveTo(this.plotArea.x, this.plotArea.y);
    ctx.lineTo(this.plotArea.x, this.plotArea.y + this.plotArea.height);
    ctx.stroke();
    
    // Draw labels
    ctx.fillStyle = this.options.labelColor;
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'center';
    
    // X-axis labels
    const xSteps = 5;
    for (let i = 0; i <= xSteps; i++) {
      const value = minMax.minX + (minMax.maxX - minMax.minX) * i / xSteps;
      const x = this.plotArea.x + (this.plotArea.width * i / xSteps);
      const y = this.plotArea.y + this.plotArea.height;
      
      // Tick
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x, y + 5);
      ctx.stroke();
      
      // Label
      ctx.fillText(value.toFixed(1), x, y + 15);
    }
    
    // Y-axis labels
    const ySteps = 5;
    ctx.textAlign = 'right';
    for (let i = 0; i <= ySteps; i++) {
      const value = minMax.minY + (minMax.maxY - minMax.minY) * i / ySteps;
      const x = this.plotArea.x;
      const y = this.plotArea.y + this.plotArea.height - (this.plotArea.height * i / ySteps);
      
      // Tick
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x - 5, y);
      ctx.stroke();
      
      // Label
      ctx.fillText(value.toFixed(1), x - 8, y + 3);
    }
    
    // Draw grid if enabled
    if (this.options.showGrid) {
      ctx.strokeStyle = this.options.gridColor;
      
      // X grid lines
      for (let i = 1; i <= xSteps; i++) {
        const x = this.plotArea.x + (this.plotArea.width * i / xSteps);
        ctx.beginPath();
        ctx.moveTo(x, this.plotArea.y);
        ctx.lineTo(x, this.plotArea.y + this.plotArea.height);
        ctx.stroke();
      }
      
      // Y grid lines
      for (let i = 1; i <= ySteps; i++) {
        const y = this.plotArea.y + this.plotArea.height - (this.plotArea.height * i / ySteps);
        ctx.beginPath();
        ctx.moveTo(this.plotArea.x, y);
        ctx.lineTo(this.plotArea.x + this.plotArea.width, y);
        ctx.stroke();
      }
    }
    
    // Draw axis labels
    ctx.font = '12px sans-serif';
    
    // X-axis label
    if (this.options.xLabel) {
      ctx.textAlign = 'center';
      ctx.fillText(
        this.options.xLabel,
        this.plotArea.x + this.plotArea.width / 2,
        this.options.height - 5
      );
    }
    
    // Y-axis label
    if (this.options.yLabel) {
      ctx.save();
      ctx.translate(10, this.plotArea.y + this.plotArea.height / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.textAlign = 'center';
      ctx.fillText(this.options.yLabel, 0, 0);
      ctx.restore();
    }
    
    // Draw title
    if (this.options.title) {
      ctx.textAlign = 'center';
      ctx.font = 'bold 14px sans-serif';
      ctx.fillText(
        this.options.title,
        this.plotArea.x + this.plotArea.width / 2,
        this.plotArea.y - 25
      );
    }
    
    // If we have retained data, show the scrollbar position indicator
    if (this.options.retainData) {
      let maxDataLength = 0;
      this.datasets.forEach(dataset => {
        maxDataLength = Math.max(maxDataLength, dataset.data.length);
      });
      
      if (maxDataLength > this.viewWindow.size) {
        const totalWidth = this.plotArea.width;
        const scrollBarHeight = 5;
        const scrollBarY = this.plotArea.y + this.plotArea.height + 25;
        
        // Draw scroll track
        ctx.fillStyle = '#EEEEEE';
        ctx.fillRect(this.plotArea.x, scrollBarY, totalWidth, scrollBarHeight);
        
        // Calculate thumb position and width
        const thumbRatio = this.viewWindow.size / maxDataLength;
        const thumbWidth = Math.max(20, totalWidth * thumbRatio);
        const thumbPosition = this.plotArea.x + (totalWidth - thumbWidth) * (this.viewWindow.start / (maxDataLength - this.viewWindow.size));
        
        // Draw thumb
        ctx.fillStyle = this.autoScroll ? '#66CC66' : '#999999';
        ctx.fillRect(thumbPosition, scrollBarY, thumbWidth, scrollBarHeight);
        
        // Draw data range info
        ctx.fillStyle = this.options.labelColor;
        ctx.font = '10px sans-serif';
        ctx.textAlign = 'left';
        const autoScrollIndicator = this.autoScroll ? ' (Auto-scroll ON)' : '';
        ctx.fillText(
          `Showing ${this.viewWindow.start + 1} - ${Math.min(this.viewWindow.start + this.viewWindow.size, maxDataLength)} of ${maxDataLength} points${autoScrollIndicator}`,
          this.plotArea.x,
          scrollBarY + 15
        );
      }
    }
  }
  
  draw() {
    const ctx = this.ctx;
    
    // Clear canvas
    ctx.fillStyle = this.options.backgroundColor;
    ctx.fillRect(0, 0, this.options.width, this.options.height);
    
    if (this.datasets.length === 0) return this;
    
    // Find min/max values
    const minMax = this.findMinMax();
    
    // Draw axes
    this.drawAxes(minMax);
    
    // Draw each dataset
    this.datasets.forEach((dataset, index) => {
      if (dataset.data.length < 2) return;
      
      ctx.strokeStyle = dataset.color;
      ctx.lineWidth = this.options.lineWidth;
      ctx.beginPath();
      
      // Calculate the view window range
      const start = this.options.retainData ? this.viewWindow.start : 0;
      const end = this.options.retainData ? 
        Math.min(start + this.viewWindow.size, dataset.data.length) : 
        dataset.data.length;
      
      if (start >= dataset.data.length || end <= start) return;
      
      const startPoint = this.scalePoint(dataset.data[start], minMax);
      ctx.moveTo(startPoint.x, startPoint.y);
      
      // Connect points with lines
      for (let i = start + 1; i < end; i++) {
        const point = this.scalePoint(dataset.data[i], minMax);
        ctx.lineTo(point.x, point.y);
      }
      
      ctx.stroke();
      
      // Draw dots at data points if dot size > 0
      if (this.options.dotSize > 0) {
        ctx.fillStyle = dataset.color;
        for (let i = start; i < end; i++) {
          const scaledPoint = this.scalePoint(dataset.data[i], minMax);
          ctx.beginPath();
          ctx.arc(scaledPoint.x, scaledPoint.y, this.options.dotSize, 0, Math.PI * 2);
          ctx.fill();
        }
      }
    });
    
    // Draw legend if there are labels
    const hasLabels = this.datasets.some(d => d.label);
    if (hasLabels) {
      const legendX = this.plotArea.x;
      const legendY = this.plotArea.y - 15;
      
      ctx.textAlign = 'left';
      ctx.font = '10px sans-serif';
      
      let currentX = legendX;
      
      this.datasets.forEach((dataset, index) => {
        if (!dataset.label) return;
        
        const textWidth = ctx.measureText(dataset.label).width;
        if (currentX + textWidth + 30 > this.plotArea.x + this.plotArea.width) return;
        
        // Line
        ctx.strokeStyle = dataset.color;
        ctx.lineWidth = this.options.lineWidth;
        ctx.beginPath();
        ctx.moveTo(currentX, legendY);
        ctx.lineTo(currentX + 15, legendY);
        ctx.stroke();
        
        // Label
        ctx.fillStyle = this.options.labelColor;
        ctx.fillText(dataset.label, currentX + 20, legendY + 3);
        
        currentX += textWidth + 40;
      });
    }
    
    return this;
  }
  
  // Get total number of data points
  getDataLength() {
    let maxLength = 0;
    this.datasets.forEach(dataset => {
      maxLength = Math.max(maxLength, dataset.data.length);
    });
    return maxLength;
  }
}

// Expose to global scope
window.TinyLinePlot = TinyLinePlot;
