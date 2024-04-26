#version 400

const float PI = 3.14159265358979323846;

uniform float LineWidth;
uniform vec4 LineColor;
uniform vec4 LightPosition;
uniform vec3 LightIntensity;
uniform vec3 Kd;

uniform mat4 ModelViewMatrix;

uniform float time;

noperspective in vec3 EdgeDistance;
in vec3 Normal;
in vec4 Position;

struct MaterialInfo {
  float Rough;     // Roughness
  bool Metal;      // Metallic (true) or dielectric (false)
  vec3 Color;      // Diffuse color for dielectrics, f0 for metallic
} Material;

layout ( location = 0 ) out vec4 FragColor;

#define M_PI 3.14159265358979323846

float rand(vec2 co){return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);}
float rand (vec2 co, float l) {return rand(vec2(rand(co), l));}
float rand (vec2 co, float l, float t) {return rand(vec2(rand(co, l), t));}

float perlin(vec2 p, float dim, float time) {
	vec2 pos = floor(p * dim);
	vec2 posx = pos + vec2(1.0, 0.0);
	vec2 posy = pos + vec2(0.0, 1.0);
	vec2 posxy = pos + vec2(1.0);
	
	float c = rand(pos, dim, time);
	float cx = rand(posx, dim, time);
	float cy = rand(posy, dim, time);
	float cxy = rand(posxy, dim, time);
	
	vec2 d = fract(p * dim);
	d = -0.5 * cos(d * M_PI) + 0.5;
	
	float ccx = mix(c, cx, d.x);
	float cycxy = mix(cy, cxy, d.x);
	float center = mix(ccx, cycxy, d.y);
	
	return center * 2.0 - 1.0;
}

float ggxDistribution( float nDotH ) {
  float alpha2 = Material.Rough * Material.Rough * Material.Rough * Material.Rough;
  float d = (nDotH * nDotH) * (alpha2 - 1) + 1;
  return alpha2 / (PI * d * d);
}

float geomSmith( float dotProd ) {
  float k = (Material.Rough + 1.0) * (Material.Rough + 1.0) / 8.0;
  float denom = dotProd * (1 - k) + k;
  return 1.0 / denom;
}

vec3 schlickFresnel( float lDotH ) {
  vec3 f0 = vec3(0.04);
  if( Material.Metal ) {
    f0 = Material.Color;
  }
  return f0 + (1 - f0) * pow(1.0 - lDotH, 5);
}

vec3 microfacetModel(vec3 position, vec3 n ) {  
  vec3 diffuseBrdf = vec3(0.0);  // Metallic
  if( !Material.Metal ) {
    diffuseBrdf = Material.Color;
  }

  vec3 l = vec3(0.0), 
    lightI = LightIntensity * 1;
  if( LightPosition.w == 0.0 ) { // Directional light
    l = normalize(LightPosition.xyz);
  } else {                                  // Positional light
    l = LightPosition.xyz - position;
    float dist = length(l);
    l = normalize(l);
    lightI /= (dist * dist);
  }

  vec3 v = normalize( -position );
  vec3 h = normalize( v + l );
  float nDotH = dot( n, h );
  float lDotH = dot( l, h );
  float nDotL = max( dot( n, l ), 0.0 );
  float nDotV = dot( n, v );
  vec3 specBrdf = 0.25 * ggxDistribution(nDotH) * schlickFresnel(lDotH) * geomSmith(nDotL) * geomSmith(nDotV);

  return (diffuseBrdf + PI * specBrdf) * lightI * nDotL;
}

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float clamp(float v, float min, float max){
  if(v > max){v=max;}
  if(v < min){v=min;}
  return v;
}

void main()
{

    //vec4 color = vec4( diffuseModel( Position.xyz, Normal ), 1.0);
    //color = pow( color, vec4(1.0/2.2) );
    //FragColor = mix( color, LineColor, mixVal );
    //FragColor = color;

    Material.Rough = 0.1;
    Material.Metal = false;

    vec3 c1 = vec3(0.0/255.0,123.0/255.0,144.0/255.0);
    vec3 c2 = vec3(2.0/255.0,221.0/255.0,216.0/255.0);
    //calculate world position
    vec3 worldPos = vec3(inverse(ModelViewMatrix)*Position);

    //calculate height lerp value
    float ymix = map(worldPos.y, -3, 8, 0, 1);
    ymix = clamp(ymix, 0,1);
    ymix=ymix*ymix;

    //calculate color and replace color and reflectivity based on noise
    float ptest = (perlin(vec2(worldPos.x,worldPos.z), 0.5, time*0.00000001) + perlin(vec2(worldPos.x,worldPos.z), 0.2, time*0.0000001)) * 0.5;
    if(ptest < 0){
      ptest = 0;
    }
    if(ymix >= 0.95f - (ptest*0.25)){
      Material.Color = vec3(0.9,0.9,1.0);
      Material.Rough = 0.5;
    }else{
      Material.Color = mix(c1, c2, ymix); 
    }

    vec3 surfaceColor = vec3(0);
    vec3 n = vec3(normalize(Normal));
    vec3 pos = vec3(Position);
    vec3 ambient = vec3(0.01);

    surfaceColor = microfacetModel(pos, n) + ambient;
    // Gamma
    surfaceColor = pow( surfaceColor, vec3(1.0/2.2) );
    FragColor = vec4(surfaceColor, 1);

    //float ptest = perlin(vec2(worldPos.x,worldPos.z), 0.1, time*0.0000001);
    //FragColor = vec4(ptest,ptest,ptest,1.0);
}
