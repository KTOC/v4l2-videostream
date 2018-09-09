varying mediump vec2 v_TexCoord;
uniform sampler2D tex0;
uniform mediump float u_width_coef;
void main() {
    mediump vec4 yuyv = texture2D(tex0, v_TexCoord);
    if (mod(gl_FragCoord.x, 2.0) >= 1.0)
        yuyv[0] = yuyv[2];
    mediump vec3 yuv =
        vec3(1.1643 * (yuyv[0] - 0.0625), yuyv[1] - 0.5, yuyv[3] - 0.5);
    mediump vec3 rgb =
        yuv * mat3(1.0, 0, 1.5958, 1.0, -0.39173, -0.81290, 1.0, 2.017, 0);
    gl_FragColor = vec4(rgb, 1.0);
    // gl_FragColor = texture2D(tex0, v_TexCoord);
}
