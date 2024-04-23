#version 400

layout( vertices=12 ) out;

uniform int TessLevel;
uniform mat4 view;
uniform mat4 model;

uniform mat4 ModelViewMatrix;

void main()
{
    // Pass along the vertex position unmodified
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    // gl_TessLevelOuter[0] = float(TessLevel);
    // gl_TessLevelOuter[1] = float(TessLevel);
    // gl_TessLevelOuter[2] = float(TessLevel);
    // gl_TessLevelOuter[3] = float(TessLevel);

    // gl_TessLevelInner[1] = float(TessLevel);
    // gl_TessLevelInner[0] = float(TessLevel);

    if(gl_InvocationID == 0){
        // ----------------------------------------------------------------------
        // Step 1: define constants to control tessellation parameters
        // set these as desired for your world scale
        const int MIN_TESS_LEVEL = 1;
        const int MAX_TESS_LEVEL = 16;
        const float MIN_DISTANCE = 10;
        const float MAX_DISTANCE = 400;

        // ----------------------------------------------------------------------
        // Step 2: transform each vertex into eye space
        vec4 eyeSpacePos00 = ModelViewMatrix * gl_in[0].gl_Position;
        vec4 eyeSpacePos01 = ModelViewMatrix * gl_in[1].gl_Position;
        vec4 eyeSpacePos10 = ModelViewMatrix * gl_in[2].gl_Position;
        vec4 eyeSpacePos11 = ModelViewMatrix * gl_in[3].gl_Position;

        // ----------------------------------------------------------------------
        // Step 3: "distance" from camera scaled between 0 and 1
        float distance00 = clamp((abs(eyeSpacePos00.z)-MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0);
        float distance01 = clamp((abs(eyeSpacePos01.z)-MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0);
        float distance10 = clamp((abs(eyeSpacePos10.z)-MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0);
        float distance11 = clamp((abs(eyeSpacePos11.z)-MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0);

        // ----------------------------------------------------------------------
        // Step 4: interpolate edge tessellation level based on closer vertex
        float tessLevel0 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance00) );
        float tessLevel1 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01) );
        float tessLevel2 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11) );
        float tessLevel3 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10) );

        // ----------------------------------------------------------------------
        // Step 5: set the corresponding outer edge tessellation levels
        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        // ----------------------------------------------------------------------
        // Step 6: set the inner tessellation levels to the max of the two parallel edges
        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
    }
	
}
