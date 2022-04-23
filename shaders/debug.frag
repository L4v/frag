#version 330 core

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;
flat in ivec4 BoneIds;
in vec4 BoneWeights;
out vec4 FragColor;

uniform vec3 uViewPos;
uniform int uDisplayBoneIdx;

vec2 ScaledTexCoord;

void main() {

    bool Found = false;
    for(int BoneIdx = 0; BoneIdx < 4; ++BoneIdx) {
        float CurrBoneWeight = BoneWeights[BoneIdx];
        if(BoneIds[BoneIdx] == uDisplayBoneIdx) {
            if(CurrBoneWeight >= 0.7) {
                FragColor = vec4(1.0, 0.0, 0.0, 1.0) * CurrBoneWeight;
            } else if(CurrBoneWeight >= 0.4 && CurrBoneWeight < 0.7) {
                FragColor = vec4(0.0, 1.0, 0.0, 1.0) * CurrBoneWeight;
            } else if(CurrBoneWeight >= 0.1) {
                FragColor = vec4(1.0, 1.0, 0.0, 1.0) * CurrBoneWeight;
            }
            
            Found = true;
            break;
        }
    }

    if(!Found) {
        FragColor = vec4(0.0, 0.0, 1.0, 1.0);
    }
}
