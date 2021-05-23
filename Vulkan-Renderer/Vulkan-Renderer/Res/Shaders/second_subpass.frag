#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputPos; // Position output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputNormal; // Normal output from subpass 1
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inputColour; // Colour output from subpass 1
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inputDepth; // Depth output from subpass 1

layout(location = 0) out vec4 outCol;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
// ----------------------------------------------------------------------------

void main()
{
    const float depth = subpassLoad(inputDepth).r;
    const vec3 skyColour = vec3(0.1, 0.5, 0.75);
    if(depth >= 1.0)
    {
        outCol = vec4(skyColour, 1.0);
        return;
    }

    const vec3 WorldPos = subpassLoad(inputPos).rgb;
    const vec3 albedo = subpassLoad(inputColour).rgb;
    const float metallic = 0.15;
    const float roughness = 0.2;
    const vec3 lightPosition = vec3(0, 1000, 0);
    const vec3 lightColor = vec3(10000.0, 10000.0, 10000.0) * 300;
    const vec3 camPos = vec3(0.0, 0.0, 200.0);
    const float ao = 1.00;

	vec3 N = normalize(subpassLoad(inputNormal).rgb);
    vec3 V = normalize(camPos - WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // calculate per-light radiance
    vec3 L = normalize(lightPosition - WorldPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPosition - WorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
       
    vec3 nominator    = NDF * G * F; 
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
    
    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;	  

    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);        

    // add to outgoing radiance Lo
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
     
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = skyColour * vec3(0.25) * albedo * ao;

    vec3 color = ambient + Lo;   

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    const float upperBound = 1.0f;
	const float lowerBound = 0.999f;
    float depthScaled = clamp(1.0f - ((depth - lowerBound)/(upperBound - lowerBound)), 0.0, 1.0);
    color = mix(color, skyColour, 1.0f - depthScaled);

    outCol = vec4(color, 1.0);
}