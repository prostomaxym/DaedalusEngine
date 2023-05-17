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

const vec3 specStrength = vec3(0.3);
const vec3 ambientStrength = vec3(0.1);
const float shininess = 16.0f;

void main() 
{
	// get object color from texture
	vec3 objectColor = vec3(texture(tex, ex_uv).rgb);

	// ambient
	vec3 ambient = ambientStrength * objectColor;

    // diffuse
    vec3 lightDir = lightPos - ex_pos;
	float distance = length(lightDir);
	lightDir=normalize(lightDir);
	distance = distance * distance;  //n^2 distance
    vec3 normal = normalize(ex_normal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * objectColor;

    // specular
    vec3 viewDir = normalize(viewPos - ex_pos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specStrength * spec; // assuming bright white light color

    color = vec4(ambient + diffuse * lightPower/distance + specular * lightPower/distance, 1.0);
}