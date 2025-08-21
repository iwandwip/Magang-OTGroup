"use client";

import { useState } from "react";
import { Settings, Cpu, Zap, Gauge } from "lucide-react";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Separator } from "@/components/ui/separator";

import { ParameterForm } from "@/components/parameter-form";
import { SerialInterface } from "@/components/serial-interface";
import { MonitoringPanel } from "@/components/monitoring-panel";

export default function PalletizerConfigApp() {
  const [connectionStatus, setConnectionStatus] = useState("disconnected");
  const [armStatus, setArmStatus] = useState({
    arm1: "idle",
    arm2: "idle"
  });

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-50 to-slate-100 dark:from-slate-900 dark:to-slate-800">
      {/* Header */}
      <div className="border-b bg-white/50 dark:bg-slate-900/50 backdrop-blur-sm">
        <div className="container mx-auto px-4 py-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-3">
              <div className="w-10 h-10 bg-blue-600 rounded-lg flex items-center justify-center">
                <Settings className="w-6 h-6 text-white" />
              </div>
              <div>
                <h1 className="text-2xl font-bold text-slate-900 dark:text-slate-100">
                  Palletizer Configuration
                </h1>
                <p className="text-sm text-slate-600 dark:text-slate-400">
                  Industrial Parameter Management System
                </p>
              </div>
            </div>
            
            <div className="flex items-center space-x-4">
              <div className="flex items-center space-x-2">
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
              
              <div className="flex space-x-2">
                <Badge variant="outline" className="flex items-center space-x-1">
                  <Cpu className="w-3 h-3" />
                  <span>ARM1: {armStatus.arm1}</span>
                </Badge>
                <Badge variant="outline" className="flex items-center space-x-1">
                  <Cpu className="w-3 h-3" />
                  <span>ARM2: {armStatus.arm2}</span>
                </Badge>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="container mx-auto px-4 py-6">
        <Tabs defaultValue="parameters" className="space-y-6">
          <TabsList className="grid w-full grid-cols-3 lg:w-[400px]">
            <TabsTrigger value="parameters" className="flex items-center space-x-2">
              <Settings className="w-4 h-4" />
              <span>Parameters</span>
            </TabsTrigger>
            <TabsTrigger value="communication" className="flex items-center space-x-2">
              <Zap className="w-4 h-4" />
              <span>Communication</span>
            </TabsTrigger>
            <TabsTrigger value="monitoring" className="flex items-center space-x-2">
              <Gauge className="w-4 h-4" />
              <span>Monitoring</span>
            </TabsTrigger>
          </TabsList>

          {/* Parameters Tab */}
          <TabsContent value="parameters" className="space-y-6">
            <Card>
              <CardHeader>
                <CardTitle className="flex items-center space-x-2">
                  <Settings className="w-5 h-5 text-blue-600" />
                  <span>System Parameters Configuration</span>
                </CardTitle>
                <CardDescription>
                  Configure all 98 parameters for ARM LEFT, ARM RIGHT, and Global settings
                </CardDescription>
              </CardHeader>
              <CardContent>
                <ParameterForm />
              </CardContent>
            </Card>
          </TabsContent>

          {/* Communication Tab */}
          <TabsContent value="communication" className="space-y-6">
            <Card>
              <CardHeader>
                <CardTitle className="flex items-center space-x-2">
                  <Zap className="w-5 h-5 text-orange-600" />
                  <span>Serial Communication</span>
                </CardTitle>
                <CardDescription>
                  Connect to Arduino and manage parameter synchronization
                </CardDescription>
              </CardHeader>
              <CardContent>
                <SerialInterface 
                  connectionStatus={connectionStatus}
                  setConnectionStatus={setConnectionStatus}
                />
              </CardContent>
            </Card>
          </TabsContent>

          {/* Monitoring Tab */}
          <TabsContent value="monitoring" className="space-y-6">
            <Card>
              <CardHeader>
                <CardTitle className="flex items-center space-x-2">
                  <Gauge className="w-5 h-5 text-green-600" />
                  <span>System Monitoring</span>
                </CardTitle>
                <CardDescription>
                  Real-time monitoring of ARM status, sensors, and system health
                </CardDescription>
              </CardHeader>
              <CardContent>
                <MonitoringPanel 
                  armStatus={armStatus}
                  setArmStatus={setArmStatus}
                />
              </CardContent>
            </Card>
          </TabsContent>
        </Tabs>
      </div>
    </div>
  );
}