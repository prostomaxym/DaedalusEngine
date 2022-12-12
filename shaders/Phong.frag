#version 460

in vec3 ex_pos;
in vec2 ex_uv;
in vec3 ex_normal;

uniform sampler2D tex;
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform float lightPower;

out vec4 color;

const float specularStrength = 0.5;
const float ambientStrength = 0.1;

void main()
{
	//get object color from texture
	vec3 objectColor = vec3(texture(tex, ex_uv).rgb);

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(ex_normal);
    vec3 lightDir = lightPos - ex_pos;
	float distance = length(lightDir);
	distance = distance * distance;  //n^2 distance
	lightDir = normalize(lightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // specular
    vec3 viewDir = normalize(viewPos - ex_pos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse * lightPower / distance + specular * lightPower / distance) * objectColor;
    color = vec4(result, 1.0);
} 