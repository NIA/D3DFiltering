vs_1_1
dcl_position v0
dcl_texcoord v2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; c40-c44 are texcoord shifts      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

mov oPos, v0

add oT0, v2, c40
add oT1, v2, c41
add oT2, v2, c42
add oT3, v2, c43
add oT4, v2, c44
