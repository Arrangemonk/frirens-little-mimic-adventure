#version 330
in vec2 fragTexCoord;
in vec2 fragTexCoord2;
in vec3 fragPosition;
in vec4 fragColor;
uniform sampler2D texture0;
uniform sampler2D texture1;
out vec4 finalColor;

void main()
{
vec4 texelColor = texture(texture0, fragTexCoord);
vec4 texelColor2 = texture(texture1, fragTexCoord2);
finalColor = 2.0 * texelColor * texelColor2;
}