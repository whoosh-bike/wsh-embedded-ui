#include "weui_gfx_affine.h"

/* Below this a determinant is a flattened plane: the inverse would scale by more than 1e12 */
#define WEUI_GFX_AFFINE_DET_MIN 1e-12f

WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Identity(void) {
	return (WeUi_Gfx_Affine_t){.A = 1.0f, .D = 1.0f};
}

WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Translate(float x, float y) {
	return (WeUi_Gfx_Affine_t){.A = 1.0f, .D = 1.0f, .E = x, .F = y};
}

WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Scale(float x, float y) {
	return (WeUi_Gfx_Affine_t){.A = x, .D = y};
}

WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Rotate(float rad) {
	float c = cosf(rad);
	float s = sinf(rad);
	return (WeUi_Gfx_Affine_t){.A = c, .B = s, .C = -s, .D = c};
}

WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Mul(const WeUi_Gfx_Affine_t* pcM, const WeUi_Gfx_Affine_t* pcN) {
	return (WeUi_Gfx_Affine_t){
		.A = pcM->A * pcN->A + pcM->C * pcN->B,
		.B = pcM->B * pcN->A + pcM->D * pcN->B,
		.C = pcM->A * pcN->C + pcM->C * pcN->D,
		.D = pcM->B * pcN->C + pcM->D * pcN->D,
		.E = pcM->A * pcN->E + pcM->C * pcN->F + pcM->E,
		.F = pcM->B * pcN->E + pcM->D * pcN->F + pcM->F,
	};
}

WeUi_Gfx_PointF_t WeUi_Gfx_Affine_Apply(const WeUi_Gfx_Affine_t* pcM, WeUi_Gfx_PointF_t pt) {
	return (WeUi_Gfx_PointF_t){
		.X = pcM->A * pt.X + pcM->C * pt.Y + pcM->E,
		.Y = pcM->B * pt.X + pcM->D * pt.Y + pcM->F,
	};
}

RET_STATE_t WeUi_Gfx_Affine_Invert(const WeUi_Gfx_Affine_t* pcM, WeUi_Gfx_Affine_t* pInv) {
	float det = pcM->A * pcM->D - pcM->B * pcM->C;
	if (fabsf(det) < WEUI_GFX_AFFINE_DET_MIN)
		return RET_STATE_ERR_PARAM;

	float k = 1.0f / det;
	float a = pcM->D * k;
	float b = -pcM->B * k;
	float c = -pcM->C * k;
	float d = pcM->A * k;

	*pInv = (WeUi_Gfx_Affine_t){
		.A = a,
		.B = b,
		.C = c,
		.D = d,
		.E = -(a * pcM->E + c * pcM->F),
		.F = -(b * pcM->E + d * pcM->F),
	};
	return RET_STATE_SUCCESS;
}

void WeUi_Gfx_AffineStack_Reset(WeUi_Gfx_AffineStack_t* pStack) {
	pStack->Top	  = WeUi_Gfx_Affine_Identity();
	pStack->Depth = 0;
}

RET_STATE_t WeUi_Gfx_AffineStack_Push(WeUi_Gfx_AffineStack_t* pStack) {
	if (pStack->Depth >= WEUI_GFX_AFFINE_STACK_DEPTH)
		return RET_STATE_ERR_OVERFLOW;

	pStack->Saved[pStack->Depth++] = pStack->Top;
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_AffineStack_Pop(WeUi_Gfx_AffineStack_t* pStack) {
	if (pStack->Depth == 0)
		return RET_STATE_ERR_EMPTY;

	pStack->Top = pStack->Saved[--pStack->Depth];
	return RET_STATE_SUCCESS;
}

void WeUi_Gfx_AffineStack_Concat(WeUi_Gfx_AffineStack_t* pStack, const WeUi_Gfx_Affine_t* pcM) {
	pStack->Top = WeUi_Gfx_Affine_Mul(&pStack->Top, pcM);
}

void WeUi_Gfx_AffineStack_Translate(WeUi_Gfx_AffineStack_t* pStack, float x, float y) {
	WeUi_Gfx_Affine_t m = WeUi_Gfx_Affine_Translate(x, y);
	WeUi_Gfx_AffineStack_Concat(pStack, &m);
}

void WeUi_Gfx_AffineStack_Scale(WeUi_Gfx_AffineStack_t* pStack, float x, float y) {
	WeUi_Gfx_Affine_t m = WeUi_Gfx_Affine_Scale(x, y);
	WeUi_Gfx_AffineStack_Concat(pStack, &m);
}

void WeUi_Gfx_AffineStack_Rotate(WeUi_Gfx_AffineStack_t* pStack, float rad) {
	WeUi_Gfx_Affine_t m = WeUi_Gfx_Affine_Rotate(rad);
	WeUi_Gfx_AffineStack_Concat(pStack, &m);
}
