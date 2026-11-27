diff --git a/tests/DSPTests.cpp b/tests/DSPTests.cpp
--- a/tests/DSPTests.cpp
+++ b/tests/DSPTests.cpp
@@ -193,6 +193,291 @@ void testNonFiniteInputs()
     for (const float input : inputs)
     {
         float left = 1.0f, right = 1.0f;
         dsp.processFrame (input, input, true, left, right);
         require (std::isfinite (left) && std::isfinite (right), "NaN/Inf input must not propagate to wet output");
     }
+
+    p.enabled = false;
+    DistortionDSP bypass;
+    bypass.prepare (48000.0, p);
+    for (const float input : inputs)
+    {
+        float left = 1.0f, right = 1.0f;
+        bypass.processFrame (input, input, true, left, right);
+        require (left == 0.0f && right == 0.0f,
+                 "NaN/Inf input must become silence in full bypass");
+    }
+
+    p.enabled = true;
+    DistortionDSP transition;
+    p.enabled = false;
+    transition.prepare (48000.0, p);
+    p.enabled = true;
+    transition.setParameters (p);
+    for (int i = 0; i < 576; ++i)
+    {
+        float left = 0.0f, right = 0.0f;
+        transition.processFrame (std::numeric_limits<float>::quiet_NaN(),
+                                 std::numeric_limits<float>::infinity(),
+                                 true, left, right);
+        require (std::isfinite (left) && std::isfinite (right),
+                 "non-finite input must not escape during Enable ON transition");
+    }
+
+    p.enabled = false;
+    transition.setParameters (p);
+    for (int i = 0; i < 576; ++i)
+    {
+        float left = 0.0f, right = 0.0f;
+        transition.processFrame (-std::numeric_limits<float>::infinity(),
+                                 std::numeric_limits<float>::quiet_NaN(),
+                                 true, left, right);
+        require (std::isfinite (left) && std::isfinite (right),
+                 "non-finite input must not escape during Enable OFF transition");
+    }
+}
+
+void testEnabledSmoothing()
+{
+    constexpr int rampSamples = 576;
+    constexpr float inputLeft = 0.4f;
+    constexpr float inputRight = -0.3f;
+
+    DistortionDSP::Parameters p;
+    p.drive = 8.0f;
+    p.tone = 5.0f;
+    p.bias = 6.0f;
+    p.body = 5.0f;
+    p.mix = 10.0f;
+    p.enabled = false;
+
+    DistortionDSP ramp;
+    ramp.prepare (48000.0, p);
+
+    auto enabledParameters = p;
+    enabledParameters.enabled = true;
+
+    DistortionDSP reference;
+    reference.prepare (48000.0, enabledParameters);
+    ramp.setParameters (enabledParameters);
+
+    for (int i = 0; i < rampSamples; ++i)
+    {
+        float actualLeft = 0.0f, actualRight = 0.0f;
+        float wetLeft = 0.0f, wetRight = 0.0f;
+        ramp.processFrame (inputLeft, inputRight, true, actualLeft, actualRight);
+        reference.processFrame (inputLeft, inputRight, true, wetLeft, wetRight);
+
+        const float amount = static_cast<float> (i + 1) / rampSamples;
+        const float expectedLeft = inputLeft + (wetLeft - inputLeft) * amount;
+        const float expectedRight = inputRight + (wetRight - inputRight) * amount;
+
+        require (std::abs (actualLeft - expectedLeft) < 1.0e-5f,
+                 "Enable ON ramp must follow the expected linear progression (left)");
+        require (std::abs (actualRight - expectedRight) < 1.0e-5f,
+                 "Enable ON ramp must follow the expected linear progression (right)");
+    }
+
+    auto disabledParameters = enabledParameters;
+    disabledParameters.enabled = false;
+    ramp.setParameters (disabledParameters);
+
+    for (int i = 0; i < rampSamples; ++i)
+    {
+        float actualLeft = 0.0f, actualRight = 0.0f;
+        float wetLeft = 0.0f, wetRight = 0.0f;
+        ramp.processFrame (inputLeft, inputRight, true, actualLeft, actualRight);
+        reference.processFrame (inputLeft, inputRight, true, wetLeft, wetRight);
+
+        const float amount = 1.0f - static_cast<float> (i + 1) / rampSamples;
+        const float expectedLeft = inputLeft + (wetLeft - inputLeft) * amount;
+        const float expectedRight = inputRight + (wetRight - inputRight) * amount;
+
+        require (std::abs (actualLeft - expectedLeft) < 1.0e-5f,
+                 "Enable OFF ramp must follow the expected linear progression (left)");
+        require (std::abs (actualRight - expectedRight) < 1.0e-5f,
+                 "Enable OFF ramp must follow the expected linear progression (right)");
+    }
+}
+
+void testEnabledRetargetDuringRamp()
+{
+    constexpr int rampSamples = 576;
+    constexpr int firstRampSamples = 200;
+    constexpr float inputLeft = 0.4f;
+    constexpr float inputRight = -0.3f;
+
+    DistortionDSP::Parameters p;
+    p.drive = 8.0f;
+    p.tone = 5.0f;
+    p.bias = 6.0f;
+    p.body = 5.0f;
+    p.mix = 10.0f;
+    p.enabled = false;
+
+    DistortionDSP ramp;
+    ramp.prepare (48000.0, p);
+
+    auto enabledParameters = p;
+    enabledParameters.enabled = true;
+    ramp.setParameters (enabledParameters);
+
+    DistortionDSP reference;
+    reference.prepare (48000.0, enabledParameters);
+
+    for (int i = 0; i < firstRampSamples; ++i)
+    {
+        float left = 0.0f, right = 0.0f;
+        float refLeft = 0.0f, refRight = 0.0f;
+        ramp.processFrame (inputLeft, inputRight, true, left, right);
+        reference.processFrame (inputLeft, inputRight, true, refLeft, refRight);
+    }
+
+    auto disabledParameters = enabledParameters;
+    disabledParameters.enabled = false;
+    ramp.setParameters (disabledParameters);
+
+    for (int i = 0; i < rampSamples; ++i)
+    {
+        float actualLeft = 0.0f, actualRight = 0.0f;
+        float wetLeft = 0.0f, wetRight = 0.0f;
+        ramp.processFrame (inputLeft, inputRight, true, actualLeft, actualRight);
+        reference.processFrame (inputLeft, inputRight, true, wetLeft, wetRight);
+
+        const float currentAtRetarget =
+            static_cast<float> (firstRampSamples) / rampSamples;
+        const float amount = currentAtRetarget
+                           * (1.0f - static_cast<float> (i + 1) / rampSamples);
+        const float expectedLeft = inputLeft + (wetLeft - inputLeft) * amount;
+        const float expectedRight = inputRight + (wetRight - inputRight) * amount;
+
+        require (std::abs (actualLeft - expectedLeft) < 1.0e-5f,
+                 "Enable retarget must ramp from its current value (left)");
+        require (std::abs (actualRight - expectedRight) < 1.0e-5f,
+                 "Enable retarget must ramp from its current value (right)");
+    }
+}
+
+void testIntermediateMix()
+{
+    constexpr float inputLeft = 0.12f;
+    constexpr float inputRight = -0.09f;
+    constexpr std::array<float, 5> mixValues { 0.0f, 2.5f, 5.0f, 7.5f, 10.0f };
+
+    DistortionDSP::Parameters p;
+    p.drive = 4.0f;
+    p.tone = 5.0f;
+    p.bias = 5.0f;
+    p.body = 5.0f;
+    p.enabled = true;
+
+    auto dryParameters = p;
+    dryParameters.mix = 0.0f;
+    DistortionDSP dry;
+    dry.prepare (48000.0, dryParameters);
+
+    auto wetParameters = p;
+    wetParameters.mix = 10.0f;
+    DistortionDSP wet;
+    wet.prepare (48000.0, wetParameters);
+
+    float dryLeft = 0.0f, dryRight = 0.0f;
+    float wetLeft = 0.0f, wetRight = 0.0f;
+    dry.processFrame (inputLeft, inputRight, true, dryLeft, dryRight);
+    wet.processFrame (inputLeft, inputRight, true, wetLeft, wetRight);
+
+    require (std::abs (wetLeft) < 0.98f && std::abs (wetRight) < 0.98f,
+             "Mix reference must remain below the safety saturation threshold");
+
+    for (const float mixValue : mixValues)
+    {
+        auto testParameters = p;
+        testParameters.mix = mixValue;
+        DistortionDSP test;
+        test.prepare (48000.0, testParameters);
+
+        float actualLeft = 0.0f, actualRight = 0.0f;
+        test.processFrame (inputLeft, inputRight, true, actualLeft, actualRight);
+
+        const float amount = mixValue * 0.1f;
+        const float expectedLeft = dryLeft + (wetLeft - dryLeft) * amount;
+        const float expectedRight = dryRight + (wetRight - dryRight) * amount;
+
+        require (std::abs (actualLeft) < 0.98f && std::abs (actualRight) < 0.98f,
+                 "Intermediate Mix test must not enter the safety saturation region");
+        require (std::abs (actualLeft - expectedLeft) < 1.0e-5f,
+                 "Mix must use linear dry/wet interpolation (left)");
+        require (std::abs (actualRight - expectedRight) < 1.0e-5f,
+                 "Mix must use linear dry/wet interpolation (right)");
+    }
+}
+
+void testRapidParameterChanges()
+{
+    DistortionDSP::Parameters p;
+    p.drive = 0.0f;
+    p.tone = 5.0f;
+    p.bias = 5.0f;
+    p.body = 5.0f;
+    p.mix = 10.0f;
+    p.enabled = true;
+
+    DistortionDSP dsp;
+    dsp.prepare (48000.0, p);
+
+    for (int i = 0; i < 12000; ++i)
+    {
+        if (i % 7 == 0)
+        {
+            p.drive = static_cast<float> ((i / 7) % 11);
+            p.tone = static_cast<float> ((i / 7 * 3) % 11);
+            p.bias = static_cast<float> ((i / 7 * 5) % 11);
+            p.body = static_cast<float> ((i / 7 * 7) % 11);
+            p.mix = static_cast<float> ((i / 7 * 9) % 11);
+            p.enabled = ((i / 7) % 2) == 0;
+            dsp.setParameters (p);
+        }
+
+        const float input = 0.5f * std::sin (2.0 * pi * 997.0 * i / 48000.0);
+        float left = 0.0f, right = 0.0f;
+        dsp.processFrame (input, -input, true, left, right);
+        require (std::isfinite (left) && std::isfinite (right),
+                 "rapid parameter changes must never produce non-finite output");
+    }
+}
+
+void testToneStatePersistsAcrossBypass()
+{
+    DistortionDSP::Parameters p;
+    p.drive = 0.0f;
+    p.tone = 0.0f;
+    p.bias = 5.0f;
+    p.body = 5.0f;
+    p.mix = 10.0f;
+    p.enabled = true;
+
+    DistortionDSP underTest;
+    DistortionDSP control;
+    DistortionDSP resetReference;
+    underTest.prepare (48000.0, p);
+    control.prepare (48000.0, p);
+    resetReference.prepare (48000.0, p);
+
+    // Establish identical, non-zero filter states in all three instances.
+    for (int i = 0; i < 64; ++i)
+    {
+        float aL = 0.0f, aR = 0.0f;
+        float bL = 0.0f, bR = 0.0f;
+        float cL = 0.0f, cR = 0.0f;
+        underTest.processFrame (0.5f, -0.4f, true, aL, aR);
+        control.processFrame (0.5f, -0.4f, true, bL, bR);
+        resetReference.processFrame (0.5f, -0.4f, true, cL, cR);
+    }
+
+    // All instances follow the same complete OFF ramp and enter full bypass.
+    p.enabled = false;
+    underTest.setParameters (p);
+    control.setParameters (p);
+    resetReference.setParameters (p);
+
+    for (int i = 0; i < 576; ++i)
+    {
+        float aL = 0.0f, aR = 0.0f;
+        float bL = 0.0f, bR = 0.0f;
+        float cL = 0.0f, cR = 0.0f;
+        underTest.processFrame (0.0f, 0.0f, true, aL, aR);
+        control.processFrame (0.0f, 0.0f, true, bL, bR);
+        resetReference.processFrame (0.0f, 0.0f, true, cL, cR);
+        require (std::isfinite (aL) && std::isfinite (aR)
+                 && std::isfinite (bL) && std::isfinite (bR)
+                 && std::isfinite (cL) && std::isfinite (cR),
+                 "Tone-state setup and OFF ramp must remain finite");
+    }
+
+    float offLeft = 0.0f, offRight = 0.0f;
+    underTest.processFrame (0.0f, 0.0f, true, offLeft, offRight);
+    require (offLeft == 0.0f && offRight == 0.0f,
+             "full bypass must return the exact dry signal");
+
+    // Simulate elapsed bypass time: no DSP instance is processed during it.
+    // resetReference alone is reset to provide a zero-tone-state comparison.
+    resetReference.reset();
+
+    p.enabled = true;
+    underTest.setParameters (p);
+    control.setParameters (p);
+    resetReference.setParameters (p);
+
+    for (int i = 0; i < 576; ++i)
+    {
+        float aL = 0.0f, aR = 0.0f;
+        float bL = 0.0f, bR = 0.0f;
+        float cL = 0.0f, cR = 0.0f;
+        underTest.processFrame (0.0f, 0.0f, true, aL, aR);
+        control.processFrame (0.0f, 0.0f, true, bL, bR);
+        resetReference.processFrame (0.0f, 0.0f, true, cL, cR);
+
+        require (std::isfinite (aL) && std::isfinite (aR)
+                 && std::isfinite (bL) && std::isfinite (bR)
+                 && std::isfinite (cL) && std::isfinite (cR),
+                 "Tone-state re-enable ramp must remain finite");
+
+        if (i == 0)
+        {
+            require (std::abs (aL - bL) < 1.0e-6f
+                     && std::abs (aR - bR) < 1.0e-6f,
+                     "Tone state must be preserved while bypassed");
+            require (std::abs (aL - cL) > 1.0e-6f
+                     && std::abs (aR - cR) > 1.0e-6f,
+                     "Reset tone state must differ from preserved tone state");
+        }
+    }
+}
+
+void testLongDurationProcessing()
+{
+    DistortionDSP::Parameters p;
+    p.drive = 7.0f;
+    p.tone = 4.0f;
+    p.bias = 6.0f;
+    p.body = 8.0f;
+    p.mix = 10.0f;
+    p.enabled = true;
+
+    DistortionDSP dsp;
+    dsp.prepare (48000.0, p);
+
+    for (int i = 0; i < 480000; ++i)
+    {
+        const float input = 0.7f * std::sin (2.0 * pi * 83.0 * i / 48000.0);
+        float left = 0.0f, right = 0.0f;
+        dsp.processFrame (input, -input, true, left, right);
+        require (std::isfinite (left) && std::isfinite (right),
+                 "long-duration processing must remain finite");
+    }
+}
+
+void testExtremeFiniteInputs()
+{
+    const std::array<float, 7> inputs {
+        std::numeric_limits<float>::max(),
+        -std::numeric_limits<float>::max(),
+        1.0e30f,
+        -1.0e30f,
+        1.0e20f,
+        -1.0e20f,
+        -0.0f
+    };
+
+    DistortionDSP::Parameters p;
+    p.drive = 10.0f;
+    p.tone = 5.0f;
+    p.bias = 5.0f;
+    p.body = 5.0f;
+    p.mix = 10.0f;
+    p.enabled = false;
+
+    DistortionDSP bypass;
+    bypass.prepare (48000.0, p);
+    for (const float input : inputs)
+    {
+        float left = 0.0f, right = 0.0f;
+        bypass.processFrame (input, input, true, left, right);
+        require (left == input && right == input,
+                 "finite bypass samples must be preserved exactly");
+        if (input == 0.0f)
+            require (std::signbit (left) && std::signbit (right),
+                     "finite bypass must preserve the sign of negative zero");
+    }
+
+    for (const float mixValue : { 0.0f, 5.0f, 10.0f })
+    {
+        for (const float input : inputs)
+        {
+            p.mix = mixValue;
+            p.enabled = true;
+            DistortionDSP dsp;
+            dsp.prepare (48000.0, p);
+
+            float left = 0.0f, right = 0.0f;
+            dsp.processFrame (input, -input, true, left, right);
+            require (std::isfinite (left) && std::isfinite (right),
+                     "extreme finite inputs must produce finite active output");
+        }
+    }
+
+    p.mix = 10.0f;
+    p.enabled = false;
+    DistortionDSP transition;
+    transition.prepare (48000.0, p);
+    p.enabled = true;
+    transition.setParameters (p);
+
+    for (int i = 0; i < 576; ++i)
+    {
+        float left = 0.0f, right = 0.0f;
+        transition.processFrame (std::numeric_limits<float>::max(),
+                                 -std::numeric_limits<float>::max(),
+                                 true, left, right);
+        require (std::isfinite (left) && std::isfinite (right),
+                 "extreme finite inputs must remain finite during Enable ON");
+    }
+
+    p.enabled = false;
+    transition.setParameters (p);
+    for (int i = 0; i < 576; ++i)
+    {
+        float left = 0.0f, right = 0.0f;
+        transition.processFrame (std::numeric_limits<float>::max(),
+                                 -std::numeric_limits<float>::max(),
+                                 true, left, right);
+        require (std::isfinite (left) && std::isfinite (right),
+                 "extreme finite inputs must remain finite during Enable OFF");
+    }
 }

 void testParameterExtremes (std::ofstream& csv)
@@ -287,6 +572,13 @@ int main (int argc, char** argv)
     testSignalAndFrameBatchMatrix (csv);
     testDryPathAndEnable();
     testNonFiniteInputs();
+    testEnabledSmoothing();
+    testEnabledRetargetDuringRamp();
+    testIntermediateMix();
+    testRapidParameterChanges();
+    testToneStatePersistsAcrossBypass();
+    testLongDurationProcessing();
+    testExtremeFiniteInputs();
     testParameterExtremes (csv);
     benchmarkCpu (csv);