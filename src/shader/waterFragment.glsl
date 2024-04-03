#version 400

const float PI = 3.14159265358979323846;

uniform float LineWidth;
uniform vec4 LineColor;
uniform vec4 LightPosition;
uniform vec3 LightIntensity;
uniform vec3 Kd;

noperspective in vec3 EdgeDistance;
in vec3 Normal;
in vec4 Position;

struct MaterialInfo {
  float Rough;     // Roughness
  bool Metal;      // Metallic (true) or dielectric (false)
  vec3 Color;      // Diffuse color for dielectrics, f0 for metallic
} Material;

layout ( location = 0 ) out vec4 FragColor;

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
    lightI = LightIntensity * 25000;
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

void main()
{

    //vec4 color = vec4( diffuseModel( Position.xyz, Normal ), 1.0);
    //color = pow( color, vec4(1.0/2.2) );
    //FragColor = mix( color, LineColor, mixVal );
    //FragColor = color;

    Material.Rough = 0.01;
    Material.Metal = false;
    //Material.Color = vec3(0.25,0.5,1);
    Material.Color = vec3(42.0/255.0,183.0/255.0,191.0/255.0);

    vec3 surfaceColor = vec3(0);
    vec3 n = normalize(Normal);
    vec3 pos = vec3(Position);
    vec3 ambient = vec3(0.01);

    surfaceColor = microfacetModel(pos, n) + ambient;
    // Gamma
    surfaceColor = pow( surfaceColor, vec3(1.0/2.2) );
    FragColor = vec4(surfaceColor, 1);
}
