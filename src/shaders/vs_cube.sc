$input a_position, a_normal
$output v_pos, v_view, v_normal, v_color0

#include "../../common/common.sh"

void main()
{
    mat4 model = u_model[0];

    vec3 wpos = mul(model, vec4(a_position, 1.0)).xyz;

    gl_Position = mul(u_viewProj, vec4(wpos, 1.0));
    v_pos = gl_Position.xyz;

    vec3 normal = a_normal.xyz * 2.0 - 1.0;
    v_normal = mul(u_modelView, vec4(normal, 0.0)).xyz;

    v_view = mul(u_modelView, vec4(wpos, 1.0)).xyz;

    v_color0 = vec4_splat(1.0);
}
