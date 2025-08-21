"use client";

import { useState, useEffect } from "react";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import * as z from "zod";
import { Save, RotateCcw, Download, Upload, AlertCircle } from "lucide-react";

import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Form, FormControl, FormDescription, FormField, FormItem, FormLabel, FormMessage } from "@/components/ui/form";
import { Input } from "@/components/ui/input";
import { Accordion, AccordionContent, AccordionItem, AccordionTrigger } from "@/components/ui/accordion";
import { Badge } from "@/components/ui/badge";
import { Separator } from "@/components/ui/separator";
import { Alert, AlertDescription } from "@/components/ui/alert";
import { toast } from "sonner";

// Default parameter values based on firmware analysis
const defaultParameters = {
  // Global Parameters (2)
  H: 100,
  Ly: 11,
  
  // ARM LEFT Parameters (44)
  // Home Parameters (6)
  x_L: 1305,
  y1_L: 130,
  y2_L: 410,
  z_L: 1280,
  t_L: 80,
  g_L: -10,
  
  // GLAD Parameters (6)
  gp_L: 90,
  dp_L: 40,
  za_L: 250,
  zb_L: 1320,
  Z1_L: 1325,
  T90_L: 1680,
  
  // Position Arrays LEFT (32)
  XO1_L: 645, YO1_L: 310, XE1_L: 645, YE1_L: 980,
  XO2_L: 250, YO2_L: 310, XE2_L: 250, YE2_L: 980,
  XO3_L: 645, YO3_L: 65, XE3_L: 645, YE3_L: 735,
  XO4_L: 250, YO4_L: 65, XE4_L: 250, YE4_L: 735,
  XO5_L: 785, YO5_L: 735, XE5_L: 785, YE5_L: 250,
  XO6_L: 545, YO6_L: 735, XE6_L: 545, YE6_L: 250,
  XO7_L: 245, YO7_L: 735, XE7_L: 245, YE7_L: 250,
  XO8_L: 5, YO8_L: 735, XE8_L: 5, YE8_L: 250,
  
  // ARM RIGHT Parameters (44)
  // Home Parameters (6)
  x_R: 1285,
  y1_R: 130,
  y2_R: 410,
  z_R: 1280,
  t_R: 50,
  g_R: -5,
  
  // GLAD Parameters (6)
  gp_R: 95,
  dp_R: 45,
  za_R: 250,
  zb_R: 1320,
  Z1_R: 1325,
  T90_R: 1650,
  
  // Position Arrays RIGHT (32)
  XO1_R: 625, YO1_R: 310, XE1_R: 625, YE1_R: 980,
  XO2_R: 230, YO2_R: 310, XE2_R: 230, YE2_R: 980,
  XO3_R: 625, YO3_R: 65, XE3_R: 625, YE3_R: 735,
  XO4_R: 230, YO4_R: 65, XE4_R: 230, YE4_R: 735,
  XO5_R: 765, YO5_R: 735, XE5_R: 765, YE5_R: 250,
  XO6_R: 525, YO6_R: 735, XE6_R: 525, YE6_R: 250,
  XO7_R: 225, YO7_R: 735, XE7_R: 225, YE7_R: 250,
  XO8_R: -15, YO8_R: 735, XE8_R: -15, YE8_R: 250,
  
  // Y Pattern Array (8)
  y_pattern_0: 2, y_pattern_1: 1, y_pattern_2: 2, y_pattern_3: 1,
  y_pattern_4: 1, y_pattern_5: 1, y_pattern_6: 1, y_pattern_7: 1
};

// Parameter validation schema
const parameterSchema = z.object({
  // Global parameters
  H: z.number().min(1).max(1000),
  Ly: z.number().min(1).max(20),
  
  // ARM LEFT Home parameters
  x_L: z.number().min(-5000).max(5000),
  y1_L: z.number().min(-5000).max(5000),
  y2_L: z.number().min(-5000).max(5000),
  z_L: z.number().min(-5000).max(5000),
  t_L: z.number().min(-5000).max(5000),
  g_L: z.number().min(-5000).max(5000),
  
  // ARM LEFT GLAD parameters
  gp_L: z.number().min(-5000).max(5000),
  dp_L: z.number().min(-5000).max(5000),
  za_L: z.number().min(0).max(5000),
  zb_L: z.number().min(-5000).max(5000),
  Z1_L: z.number().min(-5000).max(5000),
  T90_L: z.number().min(-5000).max(5000),
  
  // ARM LEFT Position arrays
  XO1_L: z.number().min(-5000).max(5000), YO1_L: z.number().min(-5000).max(5000),
  XE1_L: z.number().min(-5000).max(5000), YE1_L: z.number().min(-5000).max(5000),
  XO2_L: z.number().min(-5000).max(5000), YO2_L: z.number().min(-5000).max(5000),
  XE2_L: z.number().min(-5000).max(5000), YE2_L: z.number().min(-5000).max(5000),
  XO3_L: z.number().min(-5000).max(5000), YO3_L: z.number().min(-5000).max(5000),
  XE3_L: z.number().min(-5000).max(5000), YE3_L: z.number().min(-5000).max(5000),
  XO4_L: z.number().min(-5000).max(5000), YO4_L: z.number().min(-5000).max(5000),
  XE4_L: z.number().min(-5000).max(5000), YE4_L: z.number().min(-5000).max(5000),
  XO5_L: z.number().min(-5000).max(5000), YO5_L: z.number().min(-5000).max(5000),
  XE5_L: z.number().min(-5000).max(5000), YE5_L: z.number().min(-5000).max(5000),
  XO6_L: z.number().min(-5000).max(5000), YO6_L: z.number().min(-5000).max(5000),
  XE6_L: z.number().min(-5000).max(5000), YE6_L: z.number().min(-5000).max(5000),
  XO7_L: z.number().min(-5000).max(5000), YO7_L: z.number().min(-5000).max(5000),
  XE7_L: z.number().min(-5000).max(5000), YE7_L: z.number().min(-5000).max(5000),
  XO8_L: z.number().min(-5000).max(5000), YO8_L: z.number().min(-5000).max(5000),
  XE8_L: z.number().min(-5000).max(5000), YE8_L: z.number().min(-5000).max(5000),
  
  // ARM RIGHT parameters (similar structure)
  x_R: z.number().min(-5000).max(5000),
  y1_R: z.number().min(-5000).max(5000),
  y2_R: z.number().min(-5000).max(5000),
  z_R: z.number().min(-5000).max(5000),
  t_R: z.number().min(-5000).max(5000),
  g_R: z.number().min(-5000).max(5000),
  gp_R: z.number().min(-5000).max(5000),
  dp_R: z.number().min(-5000).max(5000),
  za_R: z.number().min(0).max(5000),
  zb_R: z.number().min(-5000).max(5000),
  Z1_R: z.number().min(-5000).max(5000),
  T90_R: z.number().min(-5000).max(5000),
  
  // ARM RIGHT Position arrays
  XO1_R: z.number().min(-5000).max(5000), YO1_R: z.number().min(-5000).max(5000),
  XE1_R: z.number().min(-5000).max(5000), YE1_R: z.number().min(-5000).max(5000),
  XO2_R: z.number().min(-5000).max(5000), YO2_R: z.number().min(-5000).max(5000),
  XE2_R: z.number().min(-5000).max(5000), YE2_R: z.number().min(-5000).max(5000),
  XO3_R: z.number().min(-5000).max(5000), YO3_R: z.number().min(-5000).max(5000),
  XE3_R: z.number().min(-5000).max(5000), YE3_R: z.number().min(-5000).max(5000),
  XO4_R: z.number().min(-5000).max(5000), YO4_R: z.number().min(-5000).max(5000),
  XE4_R: z.number().min(-5000).max(5000), YE4_R: z.number().min(-5000).max(5000),
  XO5_R: z.number().min(-5000).max(5000), YO5_R: z.number().min(-5000).max(5000),
  XE5_R: z.number().min(-5000).max(5000), YE5_R: z.number().min(-5000).max(5000),
  XO6_R: z.number().min(-5000).max(5000), YO6_R: z.number().min(-5000).max(5000),
  XE6_R: z.number().min(-5000).max(5000), YE6_R: z.number().min(-5000).max(5000),
  XO7_R: z.number().min(-5000).max(5000), YO7_R: z.number().min(-5000).max(5000),
  XE7_R: z.number().min(-5000).max(5000), YE7_R: z.number().min(-5000).max(5000),
  XO8_R: z.number().min(-5000).max(5000), YO8_R: z.number().min(-5000).max(5000),
  XE8_R: z.number().min(-5000).max(5000), YE8_R: z.number().min(-5000).max(5000),
  
  // Y Pattern
  y_pattern_0: z.number().min(1).max(2),
  y_pattern_1: z.number().min(1).max(2),
  y_pattern_2: z.number().min(1).max(2),
  y_pattern_3: z.number().min(1).max(2),
  y_pattern_4: z.number().min(1).max(2),
  y_pattern_5: z.number().min(1).max(2),
  y_pattern_6: z.number().min(1).max(2),
  y_pattern_7: z.number().min(1).max(2),
});

export function ParameterForm() {
  const [isLoading, setIsLoading] = useState(false);
  const [hasChanges, setHasChanges] = useState(false);

  const form = useForm({
    resolver: zodResolver(parameterSchema),
    defaultValues: defaultParameters,
    mode: "onChange"
  });

  const { watch, reset, handleSubmit } = form;
  const watchedValues = watch();

  useEffect(() => {
    const subscription = watch(() => setHasChanges(true));
    return () => subscription.unsubscribe();
  }, [watch]);

  const onSubmit = async (data) => {
    setIsLoading(true);
    try {
      // Note: Direct parameter writing to Arduino requires firmware modification
      // Current firmware doesn't implement USB parameter commands in Central State Machine
      await new Promise(resolve => setTimeout(resolve, 1000));
      toast.warning("Parameters exported locally. Firmware modification required for direct Arduino upload.");
      
      // Export parameters as JSON for manual firmware integration
      const dataStr = JSON.stringify(data, null, 2);
      const dataBlob = new Blob([dataStr], { type: 'application/json' });
      const url = URL.createObjectURL(dataBlob);
      const link = document.createElement('a');
      link.href = url;
      link.download = `palletizer_parameters_${new Date().toISOString().split('T')[0]}.json`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      URL.revokeObjectURL(url);
      
      setHasChanges(false);
    } catch (error) {
      toast.error("Failed to save parameters");
    } finally {
      setIsLoading(false);
    }
  };

  const resetToDefaults = () => {
    reset(defaultParameters);
    toast.info("Parameters reset to default values");
    setHasChanges(false);
  };

  const exportParameters = () => {
    const dataStr = JSON.stringify(watchedValues, null, 2);
    const dataBlob = new Blob([dataStr], { type: 'application/json' });
    const url = URL.createObjectURL(dataBlob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'palletizer_parameters.json';
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
    toast.success("Parameters exported successfully!");
  };

  const importParameters = (event) => {
    const file = event.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
      try {
        const imported = JSON.parse(e.target?.result);
        reset(imported);
        toast.success("Parameters imported successfully!");
        setHasChanges(true);
      } catch (error) {
        toast.error("Invalid parameter file format");
      }
    };
    reader.readAsText(file);
  };

  return (
    <div className="space-y-6">
      {/* Important Notice */}
      <Alert>
        <AlertCircle className="h-4 w-4" />
        <AlertDescription>
          <strong>Note:</strong> Current firmware (PalletizerCentralStateMachine.ino) does not implement USB parameter commands. 
          Parameters are hardcoded in firmware. Use Export/Import for configuration management and manual firmware integration.
        </AlertDescription>
      </Alert>

      {/* Action Bar */}
      <div className="flex items-center justify-between p-4 bg-slate-50 dark:bg-slate-800 rounded-lg">
        <div className="flex items-center space-x-2">
          <Badge variant="outline" className="text-sm">
            98 Parameters
          </Badge>
          {hasChanges && (
            <Badge variant="secondary" className="text-sm text-orange-600">
              <AlertCircle className="w-3 h-3 mr-1" />
              Unsaved Changes
            </Badge>
          )}
        </div>
        
        <div className="flex items-center space-x-2">
          <Button 
            variant="outline" 
            size="sm" 
            onClick={exportParameters}
            className="flex items-center space-x-1"
          >
            <Download className="w-4 h-4" />
            <span>Export</span>
          </Button>
          
          <div className="relative">
            <input
              type="file"
              accept=".json"
              onChange={importParameters}
              className="absolute inset-0 w-full h-full opacity-0 cursor-pointer"
            />
            <Button 
              variant="outline" 
              size="sm"
              className="flex items-center space-x-1"
            >
              <Upload className="w-4 h-4" />
              <span>Import</span>
            </Button>
          </div>
          
          <Button 
            variant="outline" 
            size="sm" 
            onClick={resetToDefaults}
            className="flex items-center space-x-1"
          >
            <RotateCcw className="w-4 h-4" />
            <span>Reset</span>
          </Button>
          
          <Button 
            onClick={handleSubmit(onSubmit)}
            disabled={isLoading || !hasChanges}
            className="flex items-center space-x-1"
          >
            <Save className="w-4 h-4" />
            <span>{isLoading ? "Exporting..." : "Export Parameters"}</span>
          </Button>
        </div>
      </div>

      <Form {...form}>
        <form onSubmit={handleSubmit(onSubmit)} className="space-y-6">
          <Tabs defaultValue="global" className="space-y-4">
            <TabsList className="grid w-full grid-cols-4">
              <TabsTrigger value="global">Global (2)</TabsTrigger>
              <TabsTrigger value="arm-left">ARM LEFT (44)</TabsTrigger>
              <TabsTrigger value="arm-right">ARM RIGHT (44)</TabsTrigger>
              <TabsTrigger value="patterns">Y Patterns (8)</TabsTrigger>
            </TabsList>

            {/* Global Parameters Tab */}
            <TabsContent value="global" className="space-y-4">
              <Card>
                <CardHeader>
                  <CardTitle>Global System Parameters</CardTitle>
                  <CardDescription>
                    Core system settings that apply to both arms
                  </CardDescription>
                </CardHeader>
                <CardContent className="grid grid-cols-1 md:grid-cols-2 gap-4">
                  <FormField
                    control={form.control}
                    name="H"
                    render={({ field }) => (
                      <FormItem>
                        <FormLabel>Product Height (H)</FormLabel>
                        <FormControl>
                          <Input 
                            type="number" 
                            {...field} 
                            onChange={(e) => field.onChange(Number(e.target.value))}
                          />
                        </FormControl>
                        <FormDescription>Height of the product in mm</FormDescription>
                        <FormMessage />
                      </FormItem>
                    )}
                  />
                  
                  <FormField
                    control={form.control}
                    name="Ly"
                    render={({ field }) => (
                      <FormItem>
                        <FormLabel>Number of Layers (Ly)</FormLabel>
                        <FormControl>
                          <Input 
                            type="number" 
                            {...field} 
                            onChange={(e) => field.onChange(Number(e.target.value))}
                          />
                        </FormControl>
                        <FormDescription>Total number of layers to stack</FormDescription>
                        <FormMessage />
                      </FormItem>
                    )}
                  />
                </CardContent>
              </Card>
            </TabsContent>

            {/* ARM LEFT Parameters Tab */}
            <TabsContent value="arm-left" className="space-y-4">
              <ArmParameterSection 
                form={form} 
                armSuffix="_L" 
                armName="ARM LEFT"
                description="Configuration parameters for the left palletizer arm"
              />
            </TabsContent>

            {/* ARM RIGHT Parameters Tab */}
            <TabsContent value="arm-right" className="space-y-4">
              <ArmParameterSection 
                form={form} 
                armSuffix="_R" 
                armName="ARM RIGHT"
                description="Configuration parameters for the right palletizer arm"
              />
            </TabsContent>

            {/* Y Pattern Tab */}
            <TabsContent value="patterns" className="space-y-4">
              <Card>
                <CardHeader>
                  <CardTitle>Y Pattern Configuration</CardTitle>
                  <CardDescription>
                    Y-axis movement patterns for each task position (1 = y1, 2 = y2)
                  </CardDescription>
                </CardHeader>
                <CardContent>
                  <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                    {[0, 1, 2, 3, 4, 5, 6, 7].map((index) => (
                      <FormField
                        key={index}
                        control={form.control}
                        name={`y_pattern_${index}`}
                        render={({ field }) => (
                          <FormItem>
                            <FormLabel>Pattern {index + 1}</FormLabel>
                            <FormControl>
                              <Input 
                                type="number" 
                                min="1"
                                max="2"
                                {...field} 
                                onChange={(e) => field.onChange(Number(e.target.value))}
                              />
                            </FormControl>
                            <FormMessage />
                          </FormItem>
                        )}
                      />
                    ))}
                  </div>
                </CardContent>
              </Card>
            </TabsContent>
          </Tabs>
        </form>
      </Form>
    </div>
  );
}

function ArmParameterSection({ form, armSuffix, armName, description }) {
  return (
    <div className="space-y-4">
      <div className="mb-4">
        <h3 className="text-lg font-semibold">{armName}</h3>
        <p className="text-sm text-muted-foreground">{description}</p>
      </div>
      
      <Accordion type="multiple" className="space-y-2">
        {/* Home Parameters */}
        <AccordionItem value="home">
          <AccordionTrigger className="text-left">
            <div className="flex items-center justify-between w-full mr-4">
              <span>Home Parameters</span>
              <Badge variant="outline">6 parameters</Badge>
            </div>
          </AccordionTrigger>
          <AccordionContent>
            <Card>
              <CardContent className="pt-4">
                <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                  {['x', 'y1', 'y2', 'z', 't', 'g'].map((param) => (
                    <FormField
                      key={param}
                      control={form.control}
                      name={`${param}${armSuffix}`}
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel>{param.toUpperCase()}{armSuffix}</FormLabel>
                          <FormControl>
                            <Input 
                              type="number" 
                              {...field} 
                              onChange={(e) => field.onChange(Number(e.target.value))}
                            />
                          </FormControl>
                          <FormMessage />
                        </FormItem>
                      )}
                    />
                  ))}
                </div>
              </CardContent>
            </Card>
          </AccordionContent>
        </AccordionItem>

        {/* GLAD Parameters */}
        <AccordionItem value="glad">
          <AccordionTrigger className="text-left">
            <div className="flex items-center justify-between w-full mr-4">
              <span>GLAD Parameters</span>
              <Badge variant="outline">6 parameters</Badge>
            </div>
          </AccordionTrigger>
          <AccordionContent>
            <Card>
              <CardContent className="pt-4">
                <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                  {['gp', 'dp', 'za', 'zb', 'Z1', 'T90'].map((param) => (
                    <FormField
                      key={param}
                      control={form.control}
                      name={`${param}${armSuffix}`}
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel>{param}{armSuffix}</FormLabel>
                          <FormControl>
                            <Input 
                              type="number" 
                              {...field} 
                              onChange={(e) => field.onChange(Number(e.target.value))}
                            />
                          </FormControl>
                          <FormMessage />
                        </FormItem>
                      )}
                    />
                  ))}
                </div>
              </CardContent>
            </Card>
          </AccordionContent>
        </AccordionItem>

        {/* Position Arrays */}
        <AccordionItem value="positions">
          <AccordionTrigger className="text-left">
            <div className="flex items-center justify-between w-full mr-4">
              <span>Position Arrays</span>
              <Badge variant="outline">32 parameters</Badge>
            </div>
          </AccordionTrigger>
          <AccordionContent>
            <div className="space-y-4">
              {[1, 2, 3, 4, 5, 6, 7, 8].map((taskNum) => (
                <Card key={taskNum}>
                  <CardHeader className="pb-3">
                    <CardTitle className="text-base">Task {taskNum} Positions</CardTitle>
                  </CardHeader>
                  <CardContent>
                    <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                      {['XO', 'YO', 'XE', 'YE'].map((prefix) => (
                        <FormField
                          key={`${prefix}${taskNum}`}
                          control={form.control}
                          name={`${prefix}${taskNum}${armSuffix}`}
                          render={({ field }) => (
                            <FormItem>
                              <FormLabel>{prefix}{taskNum}{armSuffix}</FormLabel>
                              <FormControl>
                                <Input 
                                  type="number" 
                                  {...field} 
                                  onChange={(e) => field.onChange(Number(e.target.value))}
                                />
                              </FormControl>
                              <FormMessage />
                            </FormItem>
                          )}
                        />
                      ))}
                    </div>
                  </CardContent>
                </Card>
              ))}
            </div>
          </AccordionContent>
        </AccordionItem>
      </Accordion>
    </div>
  );
}