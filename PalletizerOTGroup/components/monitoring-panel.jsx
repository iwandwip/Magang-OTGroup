"use client";

import { useState, useEffect } from "react";
import { Activity, Cpu, Zap, Gauge, AlertTriangle, CheckCircle, XCircle } from "lucide-react";

import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Progress } from "@/components/ui/progress";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table";
import { Alert, AlertDescription } from "@/components/ui/alert";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";

const sensorData = [
  { id: "S1", name: "Product Sensor 1", pin: "A4", description: "Detects product at position 1" },
  { id: "S2", name: "Product Sensor 2", pin: "A5", description: "Detects product at position 2" },
  { id: "S3", name: "Center Position", pin: "D2", description: "ARM in center detection" },
];

const armPins = [
  { id: "ARM1_PIN", name: "ARM1 Status", pin: "D7", description: "ARM1 busy/ready status" },
  { id: "ARM2_PIN", name: "ARM2 Status", pin: "D8", description: "ARM2 busy/ready status" },
];

const systemHealth = [
  { metric: "Communication", status: "good", value: "9600 baud" },
  { metric: "EEPROM", status: "good", value: "98 params loaded" },
  { metric: "State Machine", status: "good", value: "Running" },
  { metric: "DIP Switches", status: "good", value: "ARM1:0, ARM2:0" },
];

export function MonitoringPanel({ armStatus, setArmStatus }) {
  const [sensorStates, setSensorStates] = useState({
    S1: { state: true, lastChanged: new Date() },
    S2: { state: true, lastChanged: new Date() },
    S3: { state: true, lastChanged: new Date() },
  });
  
  const [armPinStates, setArmPinStates] = useState({
    ARM1_PIN: { state: false, lastChanged: new Date() },
    ARM2_PIN: { state: false, lastChanged: new Date() },
  });

  const [systemStats, setSystemStats] = useState({
    uptime: 0,
    cyclesCompleted: 0,
    lastCommand: "GET_STATUS",
    temperature: 25,
    memoryUsage: 65,
  });

  const [commandHistory, setCommandHistory] = useState([
    { time: "14:30:15", command: "L#H(3915,390,3840,240,-30)*7F", status: "success", arm: "ARM1" },
    { time: "14:30:18", command: "L#G(1935,2205,3975,240,120,270,750,3960,2355,240)*A3", status: "success", arm: "ARM1" },
    { time: "14:30:25", command: "R#H(3855,390,3840,150,-15)*9A", status: "success", arm: "ARM2" },
    { time: "14:30:28", command: "R#G(1875,2205,3975,150,135,285,750,3960,2295,150)*B7", status: "success", arm: "ARM2" },
    { time: "14:30:35", command: "L#C*4B", status: "success", arm: "ARM1" },
  ]);

  // Simulate real-time updates
  useEffect(() => {
    const interval = setInterval(() => {
      // Update system stats
      setSystemStats(prev => ({
        ...prev,
        uptime: prev.uptime + 1,
        temperature: 25 + Math.random() * 10,
        memoryUsage: 60 + Math.random() * 20,
      }));

      // Simulate sensor changes
      if (Math.random() < 0.1) { // 10% chance
        const sensors = Object.keys(sensorStates);
        const randomSensor = sensors[Math.floor(Math.random() * sensors.length)];
        setSensorStates(prev => ({
          ...prev,
          [randomSensor]: {
            state: !prev[randomSensor].state,
            lastChanged: new Date()
          }
        }));
      }

      // Simulate ARM status changes
      if (Math.random() < 0.15) { // 15% chance
        const arms = ['arm1', 'arm2'];
        const randomArm = arms[Math.floor(Math.random() * arms.length)];
        const states = ['idle', 'moving', 'picking', 'error'];
        const currentState = armStatus[randomArm];
        const newState = states.filter(s => s !== currentState)[Math.floor(Math.random() * 3)];
        
        setArmStatus(prev => ({
          ...prev,
          [randomArm]: newState
        }));
      }
    }, 2000);

    return () => clearInterval(interval);
  }, [armStatus, setArmStatus, sensorStates]);

  const getStatusColor = (status) => {
    switch (status) {
      case 'idle': return 'bg-blue-500';
      case 'moving': return 'bg-yellow-500';
      case 'picking': return 'bg-green-500';
      case 'error': return 'bg-red-500';
      default: return 'bg-gray-500';
    }
  };

  const getStatusIcon = (status) => {
    switch (status) {
      case 'good': return <CheckCircle className="w-4 h-4 text-green-500" />;
      case 'warning': return <AlertTriangle className="w-4 h-4 text-yellow-500" />;
      case 'error': return <XCircle className="w-4 h-4 text-red-500" />;
      default: return <Activity className="w-4 h-4" />;
    }
  };

  const formatUptime = (seconds) => {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
  };

  return (
    <div className="space-y-6">
      {/* System Overview */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
        <Card>
          <CardContent className="p-4">
            <div className="flex items-center space-x-2">
              <Activity className="w-5 h-5 text-blue-500" />
              <div>
                <p className="text-sm font-medium">System Uptime</p>
                <p className="text-2xl font-bold">{formatUptime(systemStats.uptime)}</p>
              </div>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardContent className="p-4">
            <div className="flex items-center space-x-2">
              <Gauge className="w-5 h-5 text-green-500" />
              <div>
                <p className="text-sm font-medium">Cycles Completed</p>
                <p className="text-2xl font-bold">{systemStats.cyclesCompleted}</p>
              </div>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardContent className="p-4">
            <div className="flex items-center space-x-2">
              <Zap className="w-5 h-5 text-orange-500" />
              <div>
                <p className="text-sm font-medium">Temperature</p>
                <p className="text-2xl font-bold">{systemStats.temperature.toFixed(1)}°C</p>
              </div>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardContent className="p-4">
            <div className="flex items-center space-x-2">
              <Cpu className="w-5 h-5 text-purple-500" />
              <div>
                <p className="text-sm font-medium">Memory Usage</p>
                <p className="text-2xl font-bold">{systemStats.memoryUsage.toFixed(0)}%</p>
                <Progress value={systemStats.memoryUsage} className="h-1 mt-1" />
              </div>
            </div>
          </CardContent>
        </Card>
      </div>

      <Tabs defaultValue="arms" className="space-y-4">
        <TabsList>
          <TabsTrigger value="arms">ARM Status</TabsTrigger>
          <TabsTrigger value="sensors">Sensors</TabsTrigger>
          <TabsTrigger value="commands">Commands</TabsTrigger>
          <TabsTrigger value="health">System Health</TabsTrigger>
        </TabsList>

        {/* ARM Status Tab */}
        <TabsContent value="arms" className="space-y-4">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {/* ARM1 Status */}
            <Card>
              <CardHeader>
                <CardTitle className="flex items-center space-x-2">
                  <Cpu className="w-5 h-5" />
                  <span>ARM1 Status</span>
                  <Badge variant="outline" className="flex items-center space-x-1">
                    <div className={`w-2 h-2 rounded-full ${getStatusColor(armStatus.arm1)}`} />
                    <span>{armStatus.arm1.toUpperCase()}</span>
                  </Badge>
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-3">
                <div className="space-y-2">
                  <div className="flex justify-between text-sm">
                    <span>Current State:</span>
                    <span className="font-mono">{armStatus.arm1}</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Position:</span>
                    <span className="font-mono">Layer 0, Task 0</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Last Command:</span>
                    <span className="font-mono text-xs">L#H(...)</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Cycles:</span>
                    <span className="font-mono">0 / 176</span>
                  </div>
                </div>
                <Progress value={0} className="h-2" />
                <p className="text-xs text-muted-foreground">
                  Progress: 0% complete
                </p>
              </CardContent>
            </Card>

            {/* ARM2 Status */}
            <Card>
              <CardHeader>
                <CardTitle className="flex items-center space-x-2">
                  <Cpu className="w-5 h-5" />
                  <span>ARM2 Status</span>
                  <Badge variant="outline" className="flex items-center space-x-1">
                    <div className={`w-2 h-2 rounded-full ${getStatusColor(armStatus.arm2)}`} />
                    <span>{armStatus.arm2.toUpperCase()}</span>
                  </Badge>
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-3">
                <div className="space-y-2">
                  <div className="flex justify-between text-sm">
                    <span>Current State:</span>
                    <span className="font-mono">{armStatus.arm2}</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Position:</span>
                    <span className="font-mono">Layer 0, Task 0</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Last Command:</span>
                    <span className="font-mono text-xs">R#H(...)</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span>Cycles:</span>
                    <span className="font-mono">0 / 176</span>
                  </div>
                </div>
                <Progress value={0} className="h-2" />
                <p className="text-xs text-muted-foreground">
                  Progress: 0% complete
                </p>
              </CardContent>
            </Card>
          </div>
        </TabsContent>

        {/* Sensors Tab */}
        <TabsContent value="sensors" className="space-y-4">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
            {/* Product Sensors */}
            <Card>
              <CardHeader>
                <CardTitle>Product Sensors</CardTitle>
                <CardDescription>Product detection sensors status</CardDescription>
              </CardHeader>
              <CardContent>
                <Table>
                  <TableHeader>
                    <TableRow>
                      <TableHead>Sensor</TableHead>
                      <TableHead>Pin</TableHead>
                      <TableHead>State</TableHead>
                      <TableHead>Last Change</TableHead>
                    </TableRow>
                  </TableHeader>
                  <TableBody>
                    {sensorData.map((sensor) => (
                      <TableRow key={sensor.id}>
                        <TableCell className="font-medium">{sensor.name}</TableCell>
                        <TableCell className="font-mono">{sensor.pin}</TableCell>
                        <TableCell>
                          <Badge variant={sensorStates[sensor.id]?.state ? "default" : "secondary"}>
                            {sensorStates[sensor.id]?.state ? "HIGH" : "LOW"}
                          </Badge>
                        </TableCell>
                        <TableCell className="text-xs">
                          {sensorStates[sensor.id]?.lastChanged.toLocaleTimeString()}
                        </TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </CardContent>
            </Card>

            {/* ARM Status Pins */}
            <Card>
              <CardHeader>
                <CardTitle>ARM Status Pins</CardTitle>
                <CardDescription>Hardware status feedback from ARMs</CardDescription>
              </CardHeader>
              <CardContent>
                <Table>
                  <TableHeader>
                    <TableRow>
                      <TableHead>Pin</TableHead>
                      <TableHead>ARM</TableHead>
                      <TableHead>State</TableHead>
                      <TableHead>Last Change</TableHead>
                    </TableRow>
                  </TableHeader>
                  <TableBody>
                    {armPins.map((pin) => (
                      <TableRow key={pin.id}>
                        <TableCell className="font-mono">{pin.pin}</TableCell>
                        <TableCell className="font-medium">{pin.name}</TableCell>
                        <TableCell>
                          <Badge variant={armPinStates[pin.id]?.state ? "destructive" : "default"}>
                            {armPinStates[pin.id]?.state ? "BUSY" : "READY"}
                          </Badge>
                        </TableCell>
                        <TableCell className="text-xs">
                          {armPinStates[pin.id]?.lastChanged.toLocaleTimeString()}
                        </TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </CardContent>
            </Card>
          </div>
        </TabsContent>

        {/* Commands Tab */}
        <TabsContent value="commands" className="space-y-4">
          <Card>
            <CardHeader>
              <CardTitle>Recent Commands</CardTitle>
              <CardDescription>Command execution history and status</CardDescription>
            </CardHeader>
            <CardContent>
              <Table>
                <TableHeader>
                  <TableRow>
                    <TableHead>Time</TableHead>
                    <TableHead>ARM</TableHead>
                    <TableHead>Command</TableHead>
                    <TableHead>Status</TableHead>
                  </TableRow>
                </TableHeader>
                <TableBody>
                  {commandHistory.map((cmd, index) => (
                    <TableRow key={index}>
                      <TableCell className="font-mono text-xs">{cmd.time}</TableCell>
                      <TableCell>
                        <Badge variant="outline">{cmd.arm}</Badge>
                      </TableCell>
                      <TableCell className="font-mono text-xs">{cmd.command}</TableCell>
                      <TableCell>
                        <Badge variant={cmd.status === "success" ? "default" : "destructive"}>
                          {cmd.status.toUpperCase()}
                        </Badge>
                      </TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </CardContent>
          </Card>
        </TabsContent>

        {/* System Health Tab */}
        <TabsContent value="health" className="space-y-4">
          <Card>
            <CardHeader>
              <CardTitle>System Health</CardTitle>
              <CardDescription>Overall system status and diagnostics</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              {systemHealth.map((item, index) => (
                <div key={index} className="flex items-center justify-between p-3 border rounded-lg">
                  <div className="flex items-center space-x-3">
                    {getStatusIcon(item.status)}
                    <div>
                      <p className="font-medium">{item.metric}</p>
                      <p className="text-sm text-muted-foreground">{item.value}</p>
                    </div>
                  </div>
                  <Badge variant={item.status === "good" ? "default" : "secondary"}>
                    {item.status.toUpperCase()}
                  </Badge>
                </div>
              ))}
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  );
}