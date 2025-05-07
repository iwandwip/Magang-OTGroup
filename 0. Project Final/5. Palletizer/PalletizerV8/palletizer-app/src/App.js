import React, { useState, useEffect, useRef } from 'react';
import './App.css';

function App() {
  // State variables
  const [darkMode, setDarkMode] = useState(localStorage.getItem('theme') === 'dark');
  const [systemStatus, setSystemStatus] = useState('IDLE');
  const [speedAll, setSpeedAll] = useState(200);
  const [axes, setAxes] = useState([
    { id: 'x', name: 'X', speed: 200 },
    { id: 'y', name: 'Y', speed: 200 },
    { id: 'z', name: 'Z', speed: 200 },
    { id: 't', name: 'T', speed: 200 },
    { id: 'g', name: 'G', speed: 364 },
  ]);
  const [commandText, setCommandText] = useState('');
  const [selectedFile, setSelectedFile] = useState(null);
  const [fileName, setFileName] = useState('');
  const [uploadStatus, setUploadStatus] = useState(null);
  const [writeStatus, setWriteStatus] = useState(null);
  
  const fileInputRef = useRef(null);
  const eventSourceRef = useRef(null);

  // Toggle dark mode
  const toggleDarkMode = () => {
    const newMode = !darkMode;
    setDarkMode(newMode);
    localStorage.setItem('theme', newMode ? 'dark' : 'light');
    document.body.setAttribute('data-theme', newMode ? 'dark' : 'light');
  };

  // Set initial theme on mount
  useEffect(() => {
    document.body.setAttribute('data-theme', darkMode ? 'dark' : 'light');
  }, [darkMode]);

  // Connect to server events on component mount
  useEffect(() => {
    // Initial status check
    fetch('/status')
      .then(response => response.json())
      .then(data => {
        setSystemStatus(data.status);
      })
      .catch(error => {
        console.error('Error fetching status:', error);
      });

    // Connect to server events
    const es = new EventSource('/events');
    eventSourceRef.current = es;

    es.onmessage = function(event) {
      try {
        const data = JSON.parse(event.data);
        if (data.type === 'status') {
          setSystemStatus(data.value);
        }
      } catch (error) {
        console.error('Error parsing event data:', error);
      }
    };

    es.onerror = function(err) {
      console.error('EventSource error:', err);
      setTimeout(() => {
        es.close();
        const newEventSource = new EventSource('/events');
        eventSourceRef.current = newEventSource;
      }, 5000);
    };

    // Cleanup
    return () => {
      if (eventSourceRef.current) {
        eventSourceRef.current.close();
      }
    };
  }, []);

  // Handle all speeds update from slider
  const updateAllSpeedsSlider = (e) => {
    const value = parseInt(e.target.value, 10);
    if (isNaN(value)) return;
    
    setSpeedAll(value);
    // Update all axes except G which might have a different range
    setAxes(prevAxes => 
      prevAxes.map(axis => 
        axis.id !== 'g' ? { ...axis, speed: value } : axis
      )
    );
  };

  // Handle all speeds update from input
  const updateAllSpeedsInput = (e) => {
    const value = parseInt(e.target.value, 10);
    if (isNaN(value)) return;
    
    // Clamp the value between 10 and 1000
    const clampedValue = Math.max(10, Math.min(1000, value));
    
    setSpeedAll(clampedValue);
    // Update all axes except G which might have a different range
    setAxes(prevAxes => 
      prevAxes.map(axis => 
        axis.id !== 'g' ? { ...axis, speed: clampedValue } : axis
      )
    );
  };

  // Update a specific axis speed from slider
  const updateAxisSpeedSlider = (id, e) => {
    const value = parseInt(e.target.value, 10);
    if (isNaN(value)) return;
    
    setAxes(prevAxes => 
      prevAxes.map(axis => 
        axis.id === id ? { ...axis, speed: value } : axis
      )
    );
  };

  // Update a specific axis speed from input
  const updateAxisSpeedInput = (id, e) => {
    const value = parseInt(e.target.value, 10);
    if (isNaN(value)) return;
    
    // Clamp the value between 10 and 1000
    const clampedValue = Math.max(10, Math.min(1000, value));
    
    setAxes(prevAxes => 
      prevAxes.map(axis => 
        axis.id === id ? { ...axis, speed: clampedValue } : axis
      )
    );
  };

  // Set speed for a specific axis
  const setSpeed = (axisId) => {
    const axis = axes.find(a => a.id === axisId);
    if (axis) {
      sendCommand(`SPEED;${axisId};${axis.speed}`);
    }
  };

  // Set speed for all axes
  const setAllSpeeds = () => {
    sendCommand(`SPEED;${speedAll}`);
  };

  // Send command to the server
  const sendCommand = (cmd) => {
    fetch('/command', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: `cmd=${cmd}`,
    })
      .then(response => response.text())
      .then(data => {
        console.log('Command sent:', data);
      })
      .catch(error => {
        console.error('Error:', error);
      });
  };

  // Handle file selection
  const handleFileChange = (event) => {
    const file = event.target.files[0];
    setSelectedFile(file);
    setFileName(file ? file.name : '');
  };

  // Trigger file input click
  const triggerFileInput = () => {
    fileInputRef.current.click();
  };

  // Upload file to server
  const uploadFile = () => {
    if (!selectedFile) {
      setUploadStatus({
        type: 'error',
        message: 'Please select a file',
      });
      return;
    }

    setUploadStatus({
      type: 'info',
      message: 'Uploading...',
    });

    const formData = new FormData();
    formData.append('file', selectedFile);

    fetch('/upload', {
      method: 'POST',
      body: formData,
    })
      .then(response => response.text())
      .then(data => {
        console.log('Upload result:', data);
        setUploadStatus({
          type: 'success',
          message: 'File uploaded successfully',
        });
      })
      .catch(error => {
        console.error('Error:', error);
        setUploadStatus({
          type: 'error',
          message: `Error uploading file: ${error}`,
        });
      });
  };

  // Save commands to server
  const saveCommands = () => {
    if (!commandText.trim()) {
      setWriteStatus({
        type: 'error',
        message: 'Please enter commands',
      });
      return;
    }

    setWriteStatus({
      type: 'info',
      message: 'Saving...',
    });

    fetch('/write', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: `text=${encodeURIComponent(commandText)}`,
    })
      .then(response => response.text())
      .then(data => {
        console.log('Save result:', data);
        setWriteStatus({
          type: 'success',
          message: 'Commands saved and loaded successfully',
        });
      })
      .catch(error => {
        console.error('Error:', error);
        setWriteStatus({
          type: 'error',
          message: `Error saving commands: ${error}`,
        });
      });
  };

  // Get commands from server
  const getCommands = () => {
    setWriteStatus({
      type: 'info',
      message: 'Loading commands...',
    });

    fetch('/get_commands')
      .then(response => {
        if (!response.ok) {
          throw new Error('Failed to fetch commands');
        }
        return response.text();
      })
      .then(data => {
        setCommandText(data);
        setWriteStatus({
          type: 'success',
          message: 'Commands loaded successfully',
        });
      })
      .catch(error => {
        console.error('Error:', error);
        setWriteStatus({
          type: 'error',
          message: `Error loading commands: ${error}`,
        });
      });
  };

  // Download commands
  const downloadCommands = () => {
    window.location.href = '/download_commands';
  };

  // Get status class
  const getStatusClass = () => {
    switch (systemStatus) {
      case 'RUNNING': return 'status-running';
      case 'PAUSED': return 'status-paused';
      case 'IDLE': case 'STOPPING': return 'status-idle';
      default: return '';
    }
  };

  return (
    <div className="app-container">
      {/* Header */}
      <header className="header">
        <div className="header-left">
          <span className="robot-icon">🤖</span>
          <h1>ESP32 Palletizer Control</h1>
        </div>
        <div className="header-right">
          <div className="theme-toggle">
            <input 
              type="checkbox" 
              id="theme-switch" 
              className="theme-switch-input" 
              checked={darkMode}
              onChange={toggleDarkMode}
            />
            <label htmlFor="theme-switch" className="theme-switch-label">
              <span>🌞</span>
              <span>🌙</span>
              <div className="ball"></div>
            </label>
          </div>
          <div className="status-display">
            <span>Status:</span>
            <span className={`status-badge ${getStatusClass()}`}>{systemStatus}</span>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="main-content">
        {/* Control Panel */}
        <section className="panel">
          <div className="panel-header">
            <span className="panel-icon">🎮</span>
            <h2>Control Panel</h2>
          </div>
          
          <div className="control-buttons">
            <button className="btn play" onClick={() => sendCommand('PLAY')}>
              <span className="btn-icon">▶</span> PLAY
            </button>
            <button className="btn pause" onClick={() => sendCommand('PAUSE')}>
              <span className="btn-icon">⏸</span> PAUSE
            </button>
            <button className="btn stop" onClick={() => sendCommand('STOP')}>
              <span className="btn-icon">⏹</span> STOP
            </button>
            <button className="btn idle" onClick={() => sendCommand('IDLE')}>
              <span className="btn-icon">⚪</span> IDLE
            </button>
            <button className="btn zero" onClick={() => sendCommand('ZERO')}>
              <span className="btn-icon">⌂</span> ZERO
            </button>
          </div>

          <div className="speed-controls">
            <div className="speed-control">
              <div className="speed-label">All Axes Speed</div>
              <div className="speed-input-group">
                <input 
                  type="range" 
                  min="10" 
                  max="1000" 
                  value={speedAll} 
                  onChange={updateAllSpeedsSlider} 
                  className="slider"
                />
                <input 
                  type="number" 
                  value={speedAll} 
                  onChange={updateAllSpeedsInput} 
                  min="10" 
                  max="1000" 
                  className="speed-input"
                />
                <button className="btn set-btn" onClick={setAllSpeeds}>
                  <span className="check-icon">✓</span> Set All
                </button>
              </div>
            </div>

            {axes.map(axis => (
              <div className="speed-control" key={axis.id}>
                <div className="speed-label">{axis.name} Axis</div>
                <div className="speed-input-group">
                  <input 
                    type="range" 
                    min="10" 
                    max="1000" 
                    value={axis.speed} 
                    onChange={(e) => updateAxisSpeedSlider(axis.id, e)} 
                    className="slider"
                  />
                  <input 
                    type="number" 
                    value={axis.speed} 
                    onChange={(e) => updateAxisSpeedInput(axis.id, e)} 
                    min="10" 
                    max="1000" 
                    className="speed-input"
                  />
                  <button 
                    className="btn set-btn" 
                    onClick={() => setSpeed(axis.id)}
                  >
                    <span className="check-icon">✓</span> Set
                  </button>
                </div>
              </div>
            ))}
          </div>
        </section>

        {/* Upload Command File */}
        <section className="panel">
          <div className="panel-header">
            <span className="panel-icon">📤</span>
            <h2>Upload Command File</h2>
          </div>
          
          <div className="upload-area" onClick={triggerFileInput}>
            <input
              type="file"
              ref={fileInputRef}
              accept=".txt"
              onChange={handleFileChange}
              style={{ display: 'none' }}
            />
            <div className="upload-icon">📁</div>
            <p className="upload-text">Click to select a command file</p>
            <p className="file-status">
              {fileName ? `Selected: ${fileName}` : 'No file selected'}
            </p>
          </div>

          <button
            className="btn upload-btn"
            disabled={!selectedFile}
            onClick={uploadFile}
          >
            <span className="btn-icon">📤</span> Upload
          </button>

          {uploadStatus && (
            <div className={`status-message ${uploadStatus.type}`}>
              {uploadStatus.message}
            </div>
          )}
        </section>

        {/* Write Commands */}
        <section className="panel">
          <div className="panel-header">
            <span className="panel-icon">📝</span>
            <h2>Write Commands</h2>
          </div>
          
          <textarea
            value={commandText}
            onChange={(e) => setCommandText(e.target.value)}
            placeholder={`Enter commands here, one per line...
Example:
X(1,10,100),Y(1,10,100),Z(1,10,100) NEXT
X(2,20,200),Y(2,20,200),Z(2,20,200) NEXT
X(3,30,300),Y(3,30,300),Z(3,30,300)`}
            className="command-textarea"
          ></textarea>

          <div className="command-buttons">
            <button className="btn save-btn" onClick={saveCommands}>
              <span className="btn-icon">💾</span> Save & Load Commands
            </button>
            <button className="btn load-btn" onClick={getCommands}>
              <span className="btn-icon">📥</span> Load Current Commands
            </button>
            <button className="btn download-btn" onClick={downloadCommands}>
              <span className="btn-icon">📥</span> Download Commands
            </button>
          </div>

          {writeStatus && (
            <div className={`status-message ${writeStatus.type}`}>
              {writeStatus.message}
            </div>
          )}
        </section>
      </main>
    </div>
  );
}

export default App;