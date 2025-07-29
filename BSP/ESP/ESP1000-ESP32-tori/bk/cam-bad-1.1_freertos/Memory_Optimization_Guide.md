# ESP32-CAM Memory Optimization Guide

## Memory Issue Solutions

### Common Camera Initialization Errors

**Error: `cam_dma_config(301): frame buffer malloc failed`**
- **Cause**: Insufficient DRAM for camera frame buffer allocation
- **Solution**: Reduce frame size, increase JPEG quality number (lower quality), use single buffer

### Memory Optimization Strategies

#### 1. **Camera Configuration**
```cpp
// For boards WITHOUT PSRAM
config.fb_location = CAMERA_FB_IN_DRAM;
config.frame_size = FRAMESIZE_CIF;     // 400x296 - good balance
config.jpeg_quality = 15;              // Higher number = lower quality = less memory
config.fb_count = 1;                   // Single buffer saves memory
```

#### 2. **Frame Size Options (Memory Usage - Lowest to Highest)**
- `FRAMESIZE_QQVGA` (160x120) - Ultra low memory
- `FRAMESIZE_QCIF` (176x144) - Very low memory  
- `FRAMESIZE_HQVGA` (240x176) - Low memory
- `FRAMESIZE_CIF` (400x296) - Moderate memory ⭐ **Recommended for DRAM**
- `FRAMESIZE_VGA` (640x480) - High memory
- `FRAMESIZE_SVGA` (800x600) - Very high memory
- `FRAMESIZE_UXGA` (1600x1200) - Requires PSRAM

#### 3. **Task Stack Optimization**
Reduced stack sizes to free more DRAM:
```cpp
#define CAMERA_TASK_STACK_SIZE   4096  // Reduced from 8192
#define WEB_SERVER_TASK_STACK_SIZE 3072  // Reduced from 4096
#define GPIO_TASK_STACK_SIZE     1024  // Reduced from 2048
#define WIFI_TASK_STACK_SIZE     2048  // Reduced from 4096
```

### Troubleshooting Steps

#### Step 1: Check Available Memory
The code now includes `printMemoryInfo()` function that shows:
- Free Heap before and after initialization
- PSRAM availability
- Minimum free heap

#### Step 2: Progressive Settings
The code tries multiple camera configurations:
1. **Standard settings** (if PSRAM available)
2. **DRAM-optimized settings** (CIF, quality 15)
3. **Conservative settings** (QQVGA, quality 20)

#### Step 3: Memory Requirements by Frame Size
Approximate DRAM usage for JPEG compression:

| Frame Size | Resolution | Approx. DRAM |
|------------|------------|---------------|
| QQVGA      | 160x120    | ~10KB        |
| QCIF       | 176x144    | ~15KB        |
| HQVGA      | 240x176    | ~25KB        |
| CIF        | 400x296    | ~45KB        |
| VGA        | 640x480    | ~90KB        |

### ESP32-CAM Board Variants

#### AI-Thinker ESP32-CAM
- **PSRAM**: Usually 4MB
- **Recommended**: Use PSRAM settings when available

#### Generic ESP32-CAM (No PSRAM)
- **Memory**: ~320KB usable heap
- **Recommended**: CIF or smaller frame sizes
- **Critical**: Single frame buffer only

### Code Improvements in FreeRTOS Version

#### 1. **Graceful Degradation**
- System continues even if camera fails
- Multiple initialization attempts
- Conservative fallback settings

#### 2. **Memory Monitoring**
- Real-time memory status via `/status` endpoint
- Boot-time memory reporting
- Heap monitoring in web interface

#### 3. **Optimized Resource Usage**
- Reduced stack sizes
- Efficient task scheduling
- Proper memory cleanup

### Configuration Examples

#### For Maximum Compatibility (No PSRAM)
```cpp
config.fb_location = CAMERA_FB_IN_DRAM;
config.frame_size = FRAMESIZE_CIF;
config.jpeg_quality = 18;
config.fb_count = 1;
```

#### For Quality (With PSRAM)
```cpp
config.fb_location = CAMERA_FB_IN_PSRAM;
config.frame_size = FRAMESIZE_UXGA;
config.jpeg_quality = 10;
config.fb_count = 2;
```

#### Emergency Mode (Ultra Low Memory)
```cpp
config.fb_location = CAMERA_FB_IN_DRAM;
config.frame_size = FRAMESIZE_QQVGA;
config.jpeg_quality = 25;
config.fb_count = 1;
```

### Testing Your Configuration

1. **Monitor Serial Output** for memory information
2. **Check `/status` endpoint** for real-time memory usage
3. **Test image capture** to ensure functionality
4. **Monitor for crashes** during extended operation

### Additional Tips

#### Reduce Other Memory Usage
- Minimize String concatenations
- Use `F()` macro for string literals
- Avoid large local variables
- Use static allocation where possible

#### WiFi Optimization
```cpp
WiFi.setSleep(false);  // Prevent WiFi sleep issues
```

#### Web Server Optimization
- Serve images directly without storing
- Use streaming responses for large data
- Implement proper HTTP headers

### Recovery Strategies

If the system fails to initialize:

1. **Power cycle** the ESP32-CAM
2. **Check power supply** (needs stable 5V, 2A+)
3. **Verify GPIO connections**
4. **Try different camera modules**
5. **Flash with conservative settings**

## Common Runtime Errors and Fixes

### Error: `assert failed: xQueueGenericSendFromISR queue.c:1180 (pxQueue)`
**Cause**: GPIO interrupt fired before FreeRTOS queue was created
**Solution**: Fixed in latest version by:
- Creating FreeRTOS objects before GPIO setup
- Adding safety check in interrupt handler
- Proper initialization sequence

### Error: `gpio_install_isr_service(503): GPIO isr service already installed`
**Cause**: ISR service installed multiple times (usually after restart)
**Solution**: Code now handles this gracefully with proper error checking

### Error: `Camera capture failed` (repeated)
**Causes**: 
- Camera sensor not properly configured after initialization
- Insufficient memory for frame buffer allocation during capture
- Camera hardware not responding
- Wrong pin configuration

**Solutions**:
1. **Check camera test endpoint**: Visit `/camera-test` to diagnose
2. **Verify power supply**: Camera needs stable power (5V, 2A+)
3. **Check camera module connection**: Ensure ribbon cable is secure
4. **Memory optimization**: Reduce frame size further if needed
5. **Try different camera module**: Some modules may be faulty

**New debugging features added**:
- Retry mechanism in capture function (3 attempts)
- Detailed error reporting with memory status
- Camera sensor validation before capture
- Test capture during initialization
- `/camera-test` endpoint for real-time diagnostics

### Memory Corruption Issues
**Symptoms**: Random crashes, corrupted backtraces
**Causes**: 
- Stack overflow in tasks
- Race conditions without mutexes
- Interrupt handler accessing uninitialized resources

**Solutions Applied**:
- Reduced task stack sizes appropriately
- Added mutex protection for shared resources
- Safety checks in interrupt handlers
- Proper initialization order

### Initialization Order (Critical!)
The correct order is now implemented:
1. Serial and basic setup
2. **Create FreeRTOS objects** (queues, mutexes, semaphores)
3. Initialize camera
4. Setup GPIO and interrupts
5. Create and start tasks

This optimized FreeRTOS version provides much better memory management and should resolve the camera initialization issues you were experiencing.
