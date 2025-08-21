"use client";

import { useState, useEffect } from "react";
import { Plug, Unplug, RefreshCw, Send, Activity, AlertTriangle } from "lucide-react";

import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Badge } from "@/components/ui/badge";
import { Separator } from "@/components/ui/separator";
import { Alert, AlertDescription } from "@/components/ui/alert";
import { Progress } from "@/components/ui/progress";
import { ScrollArea } from "@/components/ui/scroll-area";
import { toast } from "sonner";

const mockPorts = [
  "COM3 - Arduino Uno",
  "COM4 - Arduino Mega", 
  "COM5 - USB Serial Device",
  "COM6 - CH340 Serial"
];

const arduinoCommands = [
  { cmd: "GET_PARAMS", desc: "Get all parameters from Arduino" },
  { cmd: "SAVE_PARAMS", desc: "Save parameters to EEPROM" },
  { cmd: "LOAD_PARAMS", desc: "Load parameters from EEPROM" },
  { cmd: "RESET_PARAMS", desc: "Reset to default parameters" },
  { cmd: "GET_STATUS", desc: "Get system status" },
  { cmd: "GET_SENSORS", desc: "Get sensor readings" },
  { cmd: "REBOOT", desc: "Restart Arduino" }
];

export function SerialInterface({ connectionStatus, setConnectionStatus }) {
  const [selectedPort, setSelectedPort] = useState("");
  const [baudRate, setBaudRate] = useState("9600");
  const [isConnecting, setIsConnecting] = useState(false);
  const [commandHistory, setCommandHistory] = useState([]);
  const [customCommand, setCustomCommand] = useState("");
  const [connectionStats, setConnectionStats] = useState({
    connected: false,
    bytesSent: 0,
    bytesReceived: 0,
    lastActivity: null
  });

  // Simulate connection stats update
  useEffect(() => {
    if (connectionStatus === "connected") {
      const interval = setInterval(() => {
        setConnectionStats(prev => ({
          ...prev,
          connected: true,
          lastActivity: new Date().toLocaleTimeString()
        }));
      }, 5000);
      return () => clearInterval(interval);
    }
  }, [connectionStatus]);

  const handleConnect = async () => {
    if (!selectedPort) {
      toast.error("Please select a COM port");
      return;
    }

    setIsConnecting(true);
    
    try {
      // Simulate connection process
      await new Promise(resolve => setTimeout(resolve, 2000));
      
      if (Math.random() > 0.3) { // 70% success rate
        setConnectionStatus("connected");
        toast.success(`Connected to ${selectedPort}`);
        addToHistory("SYSTEM", `Connected to ${selectedPort} at ${baudRate} baud`, "success");
        
        // Simulate getting initial status
        setTimeout(() => {
          addToHistory("ARDUINO", "System initialized - ARM Control V1.3 ready", "info");
          addToHistory("ARDUINO", "DIP Switch - ARM1: Layer 0, ARM2: Layer 0", "info");
        }, 1000);
      } else {
        throw new Error("Connection failed");
      }
    } catch (error) {
      toast.error("Failed to connect to Arduino");
      addToHistory("SYSTEM", `Connection failed: ${error.message}`, "error");
    } finally {
      setIsConnecting(false);
    }
  };

  const handleDisconnect = () => {
    setConnectionStatus("disconnected");
    toast.info("Disconnected from Arduino");
    addToHistory("SYSTEM", "Disconnected from Arduino", "warning");
    setConnectionStats({
      connected: false,
      bytesSent: 0,
      bytesReceived: 0,
      lastActivity: null
    });
  };

  const sendCommand = async (command) => {
    if (connectionStatus !== "connected") {
      toast.error("Not connected to Arduino");
      return;
    }

    addToHistory("SENT", command, "sent");
    
    // Simulate Arduino response
    setTimeout(() => {
      const mockResponses = {
        "GET_PARAMS": "Parameters loaded: 98 total (H=100, Ly=11, x_L=1305...)",
        "GET_STATUS": "Status: ARM1=IDLE, ARM2=IDLE, S1=HIGH, S2=HIGH, S3=HIGH",
        "GET_SENSORS": "Sensors: S1=1, S2=1, S3=1, ARM1_PIN=0, ARM2_PIN=0",
        "SAVE_PARAMS": "Parameters saved to EEPROM successfully",
        "LOAD_PARAMS": "Parameters loaded from EEPROM",
        "RESET_PARAMS": "Parameters reset to default values",
        "REBOOT": "System rebooting..."
      };
      
      const response = mockResponses[command] || `Response to: ${command}`;
      addToHistory("RECEIVED", response, "received");
      
      setConnectionStats(prev => ({
        ...prev,
        bytesSent: prev.bytesSent + command.length,
        bytesReceived: prev.bytesReceived + response.length,
        lastActivity: new Date().toLocaleTimeString()
      }));
    }, 500 + Math.random() * 1000);
  };

  const sendCustomCommand = () => {
    if (!customCommand.trim()) return;
    sendCommand(customCommand);
    setCustomCommand("");
  };

  const addToHistory = (type, message, status) => {
    const entry = {
      id: Date.now(),
      timestamp: new Date().toLocaleTimeString(),
      type,
      message,
      status
    };
    setCommandHistory(prev => [entry, ...prev].slice(0, 100)); // Keep last 100 entries
  };

  const refreshPorts = () => {
    toast.info("Refreshing COM ports...");
    // In real implementation, this would scan for available ports
  };

  return (
    <div className="space-y-6">
      {/* Connection Control */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <Card>
          <CardHeader>
            <CardTitle className="flex items-center space-x-2">
              <Plug className="w-5 h-5" />
              <span>Connection Settings</span>
            </CardTitle>
            <CardDescription>
              Configure serial communication with Arduino
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="grid grid-cols-1 gap-4">
              <div className="space-y-2">
                <Label htmlFor="port-select">COM Port</Label>
                <div className="flex space-x-2">
                  <Select value={selectedPort} onValueChange={setSelectedPort}>
                    <SelectTrigger>
                      <SelectValue placeholder="Select COM port" />
                    </SelectTrigger>
                    <SelectContent>
                      {mockPorts.map((port) => (
                        <SelectItem key={port} value={port}>
                          {port}
                        </SelectItem>
                      ))}
                    </SelectContent>
                  </Select>
                  <Button variant="outline" size="icon" onClick={refreshPorts}>
                    <RefreshCw className="w-4 h-4" />
                  </Button>
                </div>
              </div>
              
              <div className="space-y-2">
                <Label htmlFor="baud-rate">Baud Rate</Label>
                <Select value={baudRate} onValueChange={setBaudRate}>
                  <SelectTrigger>
                    <SelectValue />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="9600">9600</SelectItem>
                    <SelectItem value="19200">19200</SelectItem>
                    <SelectItem value="38400">38400</SelectItem>
                    <SelectItem value="57600">57600</SelectItem>
                    <SelectItem value="115200">115200</SelectItem>
                  </SelectContent>
                </Select>
              </div>
            </div>
            
            <Separator />
            
            <div className="flex space-x-2">
              {connectionStatus === "disconnected" ? (
                <Button 
                  onClick={handleConnect}
                  disabled={isConnecting || !selectedPort}
                  className="flex-1"
                >
                  {isConnecting ? (
                    <>
                      <RefreshCw className="w-4 h-4 mr-2 animate-spin" />
                      Connecting...
                    </>
                  ) : (
                    <>
                      <Plug className="w-4 h-4 mr-2" />
                      Connect
                    </>
                  )}
                </Button>
              ) : (
                <Button 
                  onClick={handleDisconnect}
                  variant="destructive"
                  className="flex-1"
                >
                  <Unplug className="w-4 h-4 mr-2" />
                  Disconnect
                </Button>
              )}
            </div>
          </CardContent>
        </Card>

        {/* Connection Status */}
        <Card>
          <CardHeader>
            <CardTitle className="flex items-center space-x-2">
              <Activity className="w-5 h-5" />
              <span>Connection Status</span>
            </CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="flex items-center justify-between">
              <span className="text-sm font-medium">Status:</span>
              <Badge 
                variant={connectionStatus === "connected" ? "default" : "secondary"}
                className="flex items-center space-x-1"
              >
                <div className={`w-2 h-2 rounded-full ${
                  connectionStatus === "connected" ? "bg-green-500" : "bg-red-500"
                }`} />
                <span>{connectionStatus === "connected" ? "Connected" : "Disconnected"}</span>
              </Badge>
            </div>
            
            {connectionStatus === "connected" && (
              <>
                <div className="space-y-2">
                  <div className="flex justify-between text-sm">
                    <span>Port:</span>
                    <span className="font-mono">{selectedPort}</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Baud Rate:</span>
                    <span className="font-mono">{baudRate}</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Bytes Sent:</span>
                    <span className="font-mono">{connectionStats.bytesSent}</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Bytes Received:</span>
                    <span className="font-mono">{connectionStats.bytesReceived}</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Last Activity:</span>
                    <span className="font-mono">{connectionStats.lastActivity || "N/A"}</span>
                  </div>
                </div>
              </>
            )}
          </CardContent>
        </Card>
      </div>

      {/* Command Interface */}
      <Card>
        <CardHeader>
          <CardTitle>Arduino Commands</CardTitle>
          <CardDescription>
            Send commands to the Arduino controller
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          {/* Quick Commands */}
          <div>
            <Label className="text-base font-medium">Quick Commands</Label>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-2 mt-2">
              {arduinoCommands.map((item) => (
                <Button
                  key={item.cmd}
                  variant="outline"
                  size="sm"
                  onClick={() => sendCommand(item.cmd)}
                  disabled={connectionStatus !== "connected"}
                  className="justify-start text-left h-auto p-3"
                  title={item.desc}
                >
                  <div>
                    <div className="font-mono text-xs">{item.cmd}</div>
                    <div className="text-xs text-muted-foreground truncate">
                      {item.desc}
                    </div>
                  </div>
                </Button>
              ))}
            </div>
          </div>
          
          <Separator />
          
          {/* Custom Command */}
          <div className="space-y-2">
            <Label htmlFor="custom-command">Custom Command</Label>
            <div className="flex space-x-2">
              <Input
                id="custom-command"
                value={customCommand}
                onChange={(e) => setCustomCommand(e.target.value)}
                placeholder="Enter custom command..."
                onKeyPress={(e) => e.key === 'Enter' && sendCustomCommand()}
                disabled={connectionStatus !== "connected"}
              />
              <Button 
                onClick={sendCustomCommand}
                disabled={connectionStatus !== "connected" || !customCommand.trim()}
              >
                <Send className="w-4 h-4" />
              </Button>
            </div>
          </div>
        </CardContent>
      </Card>

      {/* Command History */}
      <Card>
        <CardHeader>
          <CardTitle>Communication Log</CardTitle>
          <CardDescription>
            Real-time communication history with Arduino
          </CardDescription>
        </CardHeader>
        <CardContent>
          <ScrollArea className="h-64 w-full rounded border p-4">
            {commandHistory.length === 0 ? (
              <div className="text-center text-muted-foreground py-8">
                No communication yet. Connect to Arduino to start logging.
              </div>
            ) : (
              <div className="space-y-2">
                {commandHistory.map((entry) => (
                  <div key={entry.id} className="text-sm">
                    <div className="flex items-center space-x-2">
                      <span className="text-xs text-muted-foreground font-mono">
                        {entry.timestamp}
                      </span>
                      <Badge 
                        variant={
                          entry.status === "success" ? "default" :
                          entry.status === "error" ? "destructive" :
                          entry.status === "warning" ? "secondary" :
                          entry.status === "sent" ? "outline" :
                          "secondary"
                        }
                        className="text-xs"
                      >
                        {entry.type}
                      </Badge>
                      <span className={`font-mono flex-1 ${
                        entry.status === "error" ? "text-red-600" :
                        entry.status === "success" ? "text-green-600" :
                        entry.status === "sent" ? "text-blue-600" :
                        ""
                      }`}>
                        {entry.message}
                      </span>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </ScrollArea>
        </CardContent>
      </Card>
    </div>
  );
}