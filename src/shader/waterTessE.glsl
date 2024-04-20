#version 400

layout( quads ) in;

out vec3 TENormal;
out vec4 TEPosition;

uniform mat4 MVP;
uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;

// START stuff added for waves

uniform float time;

struct GerstnerWave {
    vec2 direction;
    float amplitude;
    float steepness;
    float frequency;
    float speed;
} gerstner_waves[4];

void initializeGerstnerWaves() {
    gerstner_waves[0] = GerstnerWave(vec2(0.707f, 0.707f), 1.9f, 0.9f, 0.3f, 1.2f);
    gerstner_waves[1] = GerstnerWave(vec2(-0.5f, 0.866f), 2.0f, 0.6f, 0.08f, 1.5f);
    gerstner_waves[2] = GerstnerWave(vec2(0.258f, -0.966f), 1.8f, 0.7f, 0.12f, 1.3f);
    gerstner_waves[3] = GerstnerWave(vec2(-0.866f, -0.5f), 2.2f, 0.5f, 0.15f, 1.8f);
}

vec3 gerstner_wave_normal(vec3 position, float time) {
    vec3 wave_normal = vec3(0.0, 1.0, 0.0);
    for (int i = 0; i < 4; ++i) {
        float proj = dot(position.xz, gerstner_waves[i].direction),
              phase = time * gerstner_waves[i].speed,
              psi = proj * gerstner_waves[i].frequency + phase,
              Af = gerstner_waves[i].amplitude *
                   gerstner_waves[i].frequency,
              alpha = Af * sin(psi);

        wave_normal.y -= gerstner_waves[i].steepness * alpha;

        float x = gerstner_waves[i].direction.x,
              y = gerstner_waves[i].direction.y,
              omega = Af * cos(psi);

        wave_normal.x -= x * omega;
        wave_normal.z -= y * omega;
    } return wave_normal;
}

vec3 gerstner_wave_position(vec2 position, float time) {
    vec3 wave_position = vec3(position.x, 0, position.y);
    for (int i = 0; i < 4; ++i) {
        float proj = dot(position, gerstner_waves[i].direction),
              phase = time * gerstner_waves[i].speed,
              theta = proj * gerstner_waves[i].frequency + phase,
              height = gerstner_waves[i].amplitude * sin(theta);

        wave_position.y += height;

        float maximum_width = gerstner_waves[i].steepness *
                              gerstner_waves[i].amplitude,
              width = maximum_width * cos(theta),
              x = gerstner_waves[i].direction.x,
              y = gerstner_waves[i].direction.y;

        wave_position.x += x * width;
        wave_position.z += y * width;
    } return wave_position;
}

vec3 gerstner_wave(vec2 position, float time, inout vec3 normal) {
    vec3 wave_position = gerstner_wave_position(position, time);
    normal = gerstner_wave_normal(wave_position, time);
    return wave_position; // Accumulated Gerstner Wave.
}

// END stuff added for waves

void main()
{
    initializeGerstnerWaves();

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    float mu,mv;

	float f1u,f2u,f3u,f4u;
	float f1v,f2v,f3v,f4v;

    float b1,b2,b3,b4;

	vec3 result;

    mu = u;
    mv = v;

    // Reassign
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p10 = gl_in[1].gl_Position;
    vec4 p11 = gl_in[2].gl_Position;
    vec4 p01 = gl_in[3].gl_Position;
    vec4 du00 = gl_in[4].gl_Position;
    vec4 du10 = gl_in[5].gl_Position;
    vec4 du11 = gl_in[6].gl_Position;
    vec4 du01 = gl_in[7].gl_Position;
    vec4 dv00 = gl_in[8].gl_Position;
    vec4 dv10 = gl_in[9].gl_Position;
    vec4 dv11 = gl_in[10].gl_Position;
    vec4 dv01 = gl_in[11].gl_Position;

    vec4 du,dv;

	f1u = 2*mu*mu*mu-3*mu*mu+1;
	f2u = -2*mu*mu*mu+3*mu*mu;
	f3u = mu*mu*mu-2*mu*mu+mu;
	f4u = mu*mu*mu - mu*mu;
	
	f1v = 2*mv*mv*mv-3*mv*mv+1;
	f2v = -2*mv*mv*mv+3*mv*mv;
	f3v = mv*mv*mv-2*mv*mv+mv;
	f4v = mv*mv*mv - mv*mv;

	b1 = p00.x*f1v+p01.x*f2v+dv00.x*f3v+dv01.x*f4v;
	b2 = p10.x*f1v+p11.x*f2v+dv10.x*f3v+dv11.x*f4v;
	b3 = du00.x*f1v+du01.x*f2v;
	b4 = du10.x*f1v+du11.x*f2v;

	result.x = f1u*b1+f2u*b2+f3u*b3+f4u*b4;

	b1 = p00.y*f1v+p01.y*f2v+dv00.y*f3v+dv01.y*f4v;
	b2 = p10.y*f1v+p11.y*f2v+dv10.y*f3v+dv11.y*f4v;
	b3 = du00.y*f1v+du01.y*f2v;
	b4 = du10.y*f1v+du11.y*f2v;

	result.y = f1u*b1+f2u*b2+f3u*b3+f4u*b4;

	b1 = p00.z*f1v+p01.z*f2v+dv00.z*f3v+dv01.z*f4v;
	b2 = p10.z*f1v+p11.z*f2v+dv10.z*f3v+dv11.z*f4v;
	b3 = du00.z*f1v+du01.z*f2v;
	b4 = du10.z*f1v+du11.z*f2v;

	result.z = f1u*b1+f2u*b2+f3u*b3+f4u*b4;

    //linearly interpolate the normal for simplicity
    du = (1-mu)*(1-mv)*du00+(1-mu)*(mv)*du01+(mu)*(1-mv)*du10+(mu)*(mv)*du11;
    dv = (1-mu)*(1-mv)*dv00+(1-mu)*(mv)*dv01+(mu)*(1-mv)*dv10+(mu)*(mv)*dv11;
    vec3 n = normalize( cross(dv.xyz, du.xyz) );

    // displace the vertices
    result = gerstner_wave(result.xz, time, n);
    TEPosition = vec4(result, 1.0);

    // Transform to clip coordinates
    gl_Position = MVP * TEPosition;

    // Convert to camera coordinates
    TEPosition = ModelViewMatrix * TEPosition;
    TENormal = normalize(NormalMatrix * n);
}
