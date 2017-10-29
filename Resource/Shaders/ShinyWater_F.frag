#version 430

// Frag output
layout( location = 0 ) out vec4 FragColor;
layout( location = 1 ) out vec4 OutColor;

layout (binding = 0) uniform sampler2D normTex;
layout (binding = 1) uniform samplerCube reflectBox;

uniform vec3 camPos;
uniform float translate = 1.0;
uniform bool underwater, fake_normals;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;
in mat3 TBN;

vec3 source_normmap(const vec2 _dir, const float speed) {
    vec2 _xy = _dir * vec2(translate) * speed;
    vec2 coord = vec2(TexCoord.x + _xy.x, TexCoord.y + _xy.y);
    vec3 norm = texture(normTex, coord).rgb;
    norm = normalize(norm * 2.0 - 1.0); // map from [0,1] to [-1, 1]
    return normalize(TBN * norm);
}

void main() {
    vec3 viewDir = normalize(FragPos - camPos);
    // grab and modify normal 
    vec3 t_norm;
    if (fake_normals) {
        t_norm = source_normmap(vec2(0.0, 1.0), 0.2);
        t_norm += source_normmap(vec2(1.0, 0.2), 0.05);
        t_norm += source_normmap(vec2(1.0, 1.0), 0.02);
        t_norm += source_normmap(vec2(-1.0, -1.0), 0.08);
        t_norm = normalize(t_norm);
    }
    else {
        t_norm = Normal;
    }

    if (underwater) { t_norm *= -1; } // flip norms if underwater

    // calculate reflection
    // -2*(V dot N)*N + V
    vec3 reflectDir = reflect(viewDir, t_norm);
    // calculate refraction
    float ratio = 1.00 / 1.33;  // ior of air divided by ior of water
    vec3 refractDir = refract(viewDir, t_norm, ratio);

    vec4 color_rfl = texture(reflectBox, reflectDir);
    vec4 color_rfr = texture(reflectBox, refractDir);
    // fresnel effect using Schlick's approximation
    float glance_angle = dot(t_norm, reflectDir);
    float R0 = pow( (1.0 - 1.33)/(1.0 + 1.33), 2);
    float fresnel = R0 + (1 - R0) * pow((1 - glance_angle), 2);
    color_rfl *= fresnel;
    color_rfr *= (1 - fresnel);
    vec4 color = color_rfl + color_rfr;

    //color.a = 0.8;

    OutColor = color;
}