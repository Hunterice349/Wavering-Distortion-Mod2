diff --git a/src/DistortionDSP.cpp b/src/DistortionDSP.cpp
--- a/src/DistortionDSP.cpp
+++ b/src/DistortionDSP.cpp
@@ -136,14 +136,15 @@ void DistortionDSP::processFrame (float inputLeft, float inputRight, bool stereo,
     const float dryLeft = finiteOrZero (inputLeft);
     const float dryRight = stereo ? finiteOrZero (inputRight) : dryLeft;

-    // A fully bypassed switch is bit-transparent, including for samples outside
-    // the plugin's normal operating range. The transition is ramped before here.
+    // Preserve finite samples exactly in bypass, without applying the effect.
+    // Non-finite input is replaced with silence so it cannot reach the output.
     if (enabled <= 0.0f)
     {
-        outputLeft = inputLeft;
-        outputRight = stereo ? inputRight : 0.0f;
+        outputLeft = dryLeft;
+        outputRight = stereo ? dryRight : 0.0f;
         return;
     }

     const float wetLeft = processChannel (dryLeft, toneStateLeft, toneCoefficient, drive, bias, body);
     const float wetRight = stereo ? processChannel (dryRight, toneStateRight, toneCoefficient, drive, bias, body)
                                   : wetLeft;

     // Preserve a true dry path at MIX=0 instead of passing it through the safety
     // saturator. The limiter only catches peaks on the processed blend.
     const float mixedLeft = mix <= 0.0f ? dryLeft : safeOutput (dryLeft + (wetLeft - dryLeft) * mix);
     const float mixedRight = mix <= 0.0f ? dryRight : safeOutput (dryRight + (wetRight - dryRight) * mix);
-    outputLeft = dryLeft + (mixedLeft - dryLeft) * enabled;
-    outputRight = stereo ? dryRight + (mixedRight - dryRight) * enabled : 0.0f;
+    outputLeft = finiteOrZero (dryLeft + (mixedLeft - dryLeft) * enabled);
+    outputRight = stereo ? finiteOrZero (dryRight + (mixedRight - dryRight) * enabled) : 0.0f;
 }