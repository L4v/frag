#version 330 core

#define POINT_LIGHT_COUNT 1

out vec4 FragColor;

struct Material {
    sampler2D Diffuse;
    sampler2D Specular;
    float Shininess;
};

struct DirLight {
    vec3 Direction;
    
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
};

struct PointLight {
    vec3 Position;

    float Constant;
    float Linear;
    float Quadratic;

    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
};

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

uniform vec3       uViewPos;
uniform DirLight   uDirLight;
uniform PointLight uPointLights[POINT_LIGHT_COUNT];
uniform Material   uMaterial;
uniform float      uTexScale;

vec2 ScaledTexCoord;

vec3
CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir);

vec3
CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    ScaledTexCoord = uTexScale * (TexCoord + 0.5) + 0.5 * uTexScale;

    // TODO(Jovan): Optimize
    vec3 NormalizedNormal = normalize(Normal);
    vec3 ViewDir = normalize(uViewPos - FragPos);

    vec3 Result = CalculateDirLight(uDirLight, NormalizedNormal, ViewDir);

    for(int PtLightIdx = 0; PtLightIdx < POINT_LIGHT_COUNT; ++PtLightIdx) {
        Result += CalculatePointLight(uPointLights[PtLightIdx], NormalizedNormal, FragPos, ViewDir);
    }

    FragColor = vec4(Result, 1.0);
}

vec3
CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 LightDir = normalize(-light.Direction);
    float Diffuse = max(dot(normal, LightDir), 0.0);
    
    vec3 ReflectDir = reflect(-LightDir, normal);
    float Specular = pow(max(dot(viewDir, ReflectDir), 0.0), uMaterial.Shininess);
    
    vec3 vAmbient = light.Ambient * vec3(texture(uMaterial.Diffuse, ScaledTexCoord));
    vec3 vDiffuse = light.Diffuse * Diffuse * vec3(texture(uMaterial.Diffuse, ScaledTexCoord));
    vec3 vSpecular = light.Specular * Specular * vec3(texture(uMaterial.Specular, ScaledTexCoord));
    
    return (vAmbient + vDiffuse + vSpecular);
    
}

vec3
CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 LightDir = normalize(light.Position - fragPos);
    float Diffuse = max(dot(normal, LightDir), 0.0);
    
    vec3 ReflectDir = reflect(-LightDir, normal);
    float Specular = pow(max(dot(viewDir, ReflectDir), 0.0), uMaterial.Shininess);

    float Distance = length(light.Position - fragPos);
    float Attenuation = 1.0 / (light.Constant + light.Linear * Distance + light.Quadratic * (Distance * Distance));
    
    vec3 vAmbient = light.Ambient * vec3(texture(uMaterial.Diffuse, ScaledTexCoord));
    vec3 vDiffuse = light.Diffuse * Diffuse * vec3(texture(uMaterial.Diffuse, ScaledTexCoord));
    vec3 vSpecular = light.Specular * Specular * vec3(texture(uMaterial.Specular, ScaledTexCoord));

    return ((vAmbient + vDiffuse + vSpecular) * Attenuation);
}
