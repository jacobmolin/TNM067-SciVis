#include "utils/structs.glsl"
// #include <string>

uniform sampler2D vfColor;
uniform ImageParameters vfParameters;
in vec3 texCoord_;

float passThrough(vec2 coord){
    return texture(vfColor,coord).x;
}

float magnitude( vec2 coord ){
    //TASK 1: find the magnitude of the vectorfield at the position coords
    vec2 velo = texture(vfColor, coord.xy).xy;
    return sqrt(pow(velo.x, 2.0) + pow(velo.y, 2.0));
    // return 0.0;
    // How can vector magnitude be extended to 3D? No idea?
    // Add a third dimension, i.e. z
}

vec2 derivative_x(vec2 coord, float delta){
    vec2 velo_pos = texture(vfColor, coord + vec2(delta, 0)).xy;
    vec2 velo_neg = texture(vfColor, coord - vec2(delta, 0)).xy;

    return (velo_pos - velo_neg) / (2.0 * delta);
}

vec2 derivative_y(vec2 coord, float delta){
    // vec2 d;
    
    // if (dim == "x") d(delta, 0);
    // else if (dim == "x") d(0, delta);

    vec2 velo_pos = texture(vfColor, coord + vec2(0, delta)).xy;
    vec2 velo_neg = texture(vfColor, coord - vec2(0, delta)).xy;

    return (velo_pos - velo_neg) / (2.0 * delta);
}

float divergence(vec2 coord){
    //TASK 2: find the divergence of the vectorfield at the position coords
    vec2 pixelSize = vfParameters.reciprocalDimensions;
    
    vec2 dVdx = derivative_x(coord, pixelSize.x);
    vec2 dVdy = derivative_y(coord, pixelSize.y);

    return dVdx.x + dVdy.y;
    // return 0.0;
    // How can vector magnitude be extended to 3D?
    // Some parts of the image are black, why? - We can't really see anything black?
}

float rotation(vec2 coord){
    //TASK 3: find the curl of the vectorfield at the position coords
    vec2 pixelSize = vfParameters.reciprocalDimensions;

	vec2 dVdxLeft = texture(vfColor, vec2(coord.x + pixelSize.x, coord.y)).xy;
	vec2 dVdxRight = texture(vfColor, vec2(coord.x - pixelSize.x, coord.y)).xy;
	vec2 dVdyLeft = texture(vfColor, vec2(coord.x, coord.y + pixelSize.y)).xy;
	vec2 dVdyRight = texture(vfColor, vec2(coord.x, coord.y - pixelSize.y)).xy;

	vec2 dVdx = (dVdxLeft - dVdxRight) / (2.0 * pixelSize.x);
	vec2 dVdy = (dVdyLeft - dVdyRight) / (2.0 * pixelSize.y);

	return dVdx.y - dVdy.x;
	// return 0.0;

    // How can vector magnitude be extended to 3D?
    // Some parts of the image are black, why? Nothing is really black...
}

void main(void) {
    float v = OUTPUT(texCoord_.xy);
    FragData0 = vec4(v);
}
