﻿/* ***************************************************************************
 * view_picture.c -- picture view
 *
 * Copyright (C) 2016-2018 by Liu Chao <lc-soft@live.cn>
 *
 * This file is part of the LC-Finder project, and may only be used, modified,
 * and distributed under the terms of the GPLv2.
 *
 * By continuing to use, modify, or distribute this file you indicate that you
 * have read the license and understand and accept it fully.
 *
 * The LC-Finder project is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL v2 for more details.
 *
 * You should have received a copy of the GPLv2 along with this file. It is
 * usually in the LICENSE.TXT file, If not, see <http://www.gnu.org/licenses/>.
 * ****************************************************************************/

/* ****************************************************************************
 * view_picture.c -- "图片" 视图，用于显示单张图片，并提供缩放、切换等功能
 *
 * 版权所有 (C) 2016-2018 归属于 刘超 <lc-soft@live.cn>
 *
 * 这个文件是 LC-Finder 项目的一部分，并且只可以根据GPLv2许可协议来使用、更改和
 * 发布。
 *
 * 继续使用、修改或发布本文件，表明您已经阅读并完全理解和接受这个许可协议。
 *
 * LC-Finder 项目是基于使用目的而加以散布的，但不负任何担保责任，甚至没有适销
 * 性或特定用途的隐含担保，详情请参照GPLv2许可协议。
 *
 * 您应已收到附随于本文件的GPLv2许可协议的副本，它通常在 LICENSE 文件中，如果
 * 没有，请查看：<http://www.gnu.org/licenses/>.
 * ****************************************************************************/

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "finder.h"
#include "ui.h"
#include "file_storage.h"

#include <LCUI/timer.h>
#include <LCUI/display.h>
#include <LCUI/cursor.h>
#include <LCUI/graph.h>
#include <LCUI/input.h>
#include <LCUI/gui/builder.h>
#include <LCUI/gui/widget.h>
#include <LCUI/gui/widget/textview.h>
#include <LCUI/font/charset.h>
#include "progressbar.h"
#include "dialog.h"
#include "i18n.h"
#include "picture.h"

#define MAX_SCALE	5.0
#define SCALE_STEP	0.333
#define MOVE_STEP	50
#define KEY_DELETE	"browser.dialog.title.confirm_delete"

#define ClearPositionStyle(W) do { \
	W->custom_style->sheet[key_left].is_valid = FALSE; \
	Widget_UnsetStyle( W, key_left ); \
	Widget_UpdateStyle( W, FALSE ); \
} while(0);

#define HideSiwtchButtons() do { \
	Widget_Hide( view.btn_prev ); \
	Widget_Hide( view.btn_next ); \
} while(0);

#define OnMouseDblClick OnBtnResetClick

enum SliderDirection {
	LEFT,
	RIGHT
};

enum SliderAction {
	SWITCH,
	RESTORE
};

enum PcitureSlot {
	PICTURE_SLOT_PREV,
	PICTURE_SLOT_CURRENT,
	PICTURE_SLOT_NEXT
};

/** 图片实例 */
typedef struct PcitureRec_ {
	wchar_t *file;			/**< 当前已加载的图片的路径  */
	wchar_t *file_for_load;		/**< 当前需加载的图片的路径  */
	LCUI_BOOL is_loading;		/**< 是否正在载入图片 */
	LCUI_BOOL is_valid;		/**< 图片内容是否有效 */
	LCUI_Graph *data;		/**< 当前已经加载的图片数据 */
	LCUI_Widget view;		/**< 视图，用于呈现该图片 */
	LCUI_Mutex mutex;		/**< 互斥锁，用于异步加载 */
	LCUI_Cond cond;			/**< 条件变量，用于异步加载 */
	double scale;			/**< 图片缩放比例 */
	double min_scale;		/**< 图片最小缩放比例 */
	int timer;			/**< 定时器，用于延迟显示“载入中...”提示框 */
} PictureRec, *Picture;

typedef struct PictureLoaderRec_ {
	int num;				/**< 当前需加载的图片的序号（下标） */
	LCUI_BOOL is_running;			/**< 是否正在运行 */
	LCUI_Cond cond;				/**< 条件变量 */
	LCUI_Mutex mutex;			/**< 互斥锁 */
	LCUI_Thread thread;			/**< 图片加载器所在的线程 */
	Picture *pictures;			/**< 图片实例引用 */
} PictureLoaderRec, *PictureLoader;

/** 图片查看器相关数据 */
static struct PictureViewer {
	LCUI_Widget window;			/**< 图片查看器窗口 */
	LCUI_Widget tip_loading;		/**< 图片载入提示框，当图片正在载入时显示 */
	LCUI_Widget tip_progress;		/**< 图片载入进度条，当图片正在载入时显示 */
	LCUI_Widget tip_empty;			/**< 提示，当无内容时显示 */
	LCUI_Widget tip_unsupport;		/**< 提示，当图片内容不受支持时显示 */
	LCUI_Widget btn_reset;			/**< 按钮，重置图片尺寸 */
	LCUI_Widget btn_prev;			/**< “上一张”按钮 */
	LCUI_Widget btn_next;			/**< “下一张”按钮 */
	LCUI_Widget btn_zoomin;			/**< “放大”按钮 */
	LCUI_Widget btn_zoomout;		/**< “缩小”按钮 */
	LCUI_Widget view_pictures;		/**< 视图，用于容纳多个图片视图 */
	LCUI_BOOL is_working;			/**< 当前视图是否在工作 */
	LCUI_BOOL is_opening;			/**< 当前视图是否为打开状态 */
	LCUI_BOOL is_zoom_mode;			/**< 当前视图是否为放大/缩小模式 */
	LCUI_BOOL is_touch_mode;		/**< 当前是否为触屏模式 */
	int mode;				/**< 当前运行模式 */
	Picture picture;			/**< 当前焦点图片 */
	Picture pictures[3];			/**< 三组图片实例 */
	FileIterator iterator;			/**< 图片文件列表迭代器 */
	int focus_x, focus_y;			/**< 当前焦点位置，相对于按比例缩放后的图片 */
	int origin_focus_x, origin_focus_y;	/**< 当前焦点位置，相对于原始尺寸的图片 */
	int offset_x, offset_y;			/**< 浏览区域的位置偏移量，相对于焦点位置 */
	/** 图片拖拽功能的相关数据 */
	struct PictureDraging {
		int focus_x, focus_y;		/**< 拖动开始时的图片坐标，相对于按比例缩放后的图片 */
		int mouse_x, mouse_y;		/**< 拖动开始时的鼠标坐标 */
		LCUI_BOOL is_running;		/**< 是否正在执行拖动操作 */
		LCUI_BOOL with_x;		/**< 拖动时是否需要改变图片X坐标 */
		LCUI_BOOL with_y;		/**< 拖动时是否需要改变图片Y坐标 */
	} drag;
	/** 手势功能的相关数据 */
	struct Gesture {
		int x, y;			/**< 当前坐标 */
		int start_x, start_y;		/**< 开始坐标 */
		int64_t timestamp;		/**< 上次坐标更新时的时间戳 */
		LCUI_BOOL is_running;		/**< 是否正在运行 */
	} gesture;
	/** 图片触控缩放功能的相关数据 */
	struct PictureTouchZoom {
		int point_ids[2];		/**< 两个触点的ID */
		int distance;			/**< 触点距离 */
		double scale;			/**< 缩放开始前的缩放比例 */
		LCUI_BOOL is_running;		/**< 缩放是否正在进行 */
		int x, y;			/**< 缩放开始前的触点位置 */
	} zoom;
	/** 图片水平滑动功能的相关数据 */
	struct SlideTransition {
		int action;			/**< 在滑动完后的行为 */
		int direction;			/**< 滚动方向 */
		int src_x;			/**< 初始 X 坐标 */
		int dst_x;			/**< 目标 X 坐标 */
		int timer;			/**< 定时器 */
		unsigned int duration;		/**< 持续时间 */
		int64_t start_time;		/**< 开始时间 */
		LCUI_BOOL is_running;		/**< 是否正在运行 */
	} slide;
	void *scanner;
	PictureLoaderRec loader;
} view = { 0 };

static void TaskForResetWidgetBackground(void *arg1, void *arg2)
{
	LCUI_Widget w = arg1;
	LCUI_Graph *image = arg2;
	Widget_UnsetStyle(w, key_background_image);
	Widget_UpdateStyle(w, FALSE);
	if (image) {
		Graph_Free(image);
		free(image);
	}
}

static void TaskForSetWidgetBackground(void *arg1, void *arg2)
{
	LCUI_Widget w = arg1;
	Widget_SetStyle(w, key_background_image, arg2, image);
	Widget_UpdateStyle(w, FALSE);
}

static void TaskForHideTipEmpty(void *arg1, void *arg2)
{
	Widget_RemoveClass(view.tip_empty, "hide");
	Widget_Show(view.tip_empty);
}

static void GetViewerSize(size_t *width, size_t *height)
{
	if (view.picture->view->width > 200) {
		*width = (size_t)view.picture->view->width - 120;
	} else {
		*width = 200;
	}
	if (view.picture->view->height > 200) {
		*height = (size_t)view.picture->view->height - 120;
	} else {
		*height = 200;
	}
}

static void PictureView_SetLabels(void)
{
	PictureLabelsViewContextRec ctx;

	ctx.file = view.picture->file;
	ctx.view = view.picture->view;
	ctx.focus_x = view.focus_x;
	ctx.focus_y = view.focus_y;
	ctx.offset_x = view.offset_x;
	ctx.offset_y = view.offset_y;
	if (view.picture && view.picture->data) {
		ctx.scale = (float)view.picture->scale;
		ctx.width = view.picture->data->width;
		ctx.height = view.picture->data->height;
	} else {
		ctx.width = 0;
		ctx.height = 0;
		ctx.scale = 1.0f;
	}
	PictureView_SetLabelsContext(&ctx);
}

/** 更新图片切换按钮的状态 */
static void UpdateSwitchButtons(void)
{
	FileIterator iter = view.iterator;
	HideSiwtchButtons();
	if (view.is_zoom_mode) {
		return;
	}
	if (iter) {
		if (iter->index > 0) {
			Widget_Show(view.btn_prev);
		}
		if (iter->length >= 1 && iter->index < iter->length - 1) {
			Widget_Show(view.btn_next);
		}
	}
}

/** 更新图片缩放按钮的状态 */
static void UpdateZoomButtons(void)
{
	if (!view.picture->is_valid) {
		Widget_SetDisabled(view.btn_zoomin, TRUE);
		Widget_SetDisabled(view.btn_zoomout, TRUE);
		return;
	}
	if (view.picture->scale > view.picture->min_scale) {
		Widget_SetDisabled(view.btn_zoomout, FALSE);
	} else {
		Widget_SetDisabled(view.btn_zoomout, TRUE);
	}
	if (view.picture->scale < MAX_SCALE) {
		Widget_SetDisabled(view.btn_zoomin, FALSE);
	} else {
		Widget_SetDisabled(view.btn_zoomin, TRUE);
	}
}

static void UpdateResetSizeButton(void)
{
	LCUI_Widget txt = LinkedList_Get(&view.btn_reset->children, 0);
	if (view.picture->is_valid) {
		Widget_SetDisabled(view.btn_reset, FALSE);
	} else {
		Widget_SetDisabled(view.btn_reset, TRUE);
	}
	if (view.picture->scale == view.picture->min_scale) {
		Widget_RemoveClass(txt, "icon-fullscreen-exit");
		Widget_AddClass(txt, "icon-fullscreen");
	} else {
		Widget_RemoveClass(txt, "icon-fullscreen");
		Widget_AddClass(txt, "icon-fullscreen-exit");
	}
}

static void SetPictureView(Picture pic)
{
	LCUI_PostSimpleTask(TaskForSetWidgetBackground,
			    pic->view, pic->data);
}

static void ClearPictureView(Picture pic)
{
	LCUI_PostSimpleTask(TaskForResetWidgetBackground,
			    pic->view, pic->data);
	pic->is_valid = FALSE;
	pic->data = NULL;
}

static int OpenPrevPicture(void)
{
	Picture pic;
	FileIterator iter = view.iterator;
	if (!iter || iter->index == 0) {
		return -1;
	}
	iter->prev(iter);
	UpdateSwitchButtons();
	pic = view.pictures[PICTURE_SLOT_NEXT];
	Widget_Prepend(view.view_pictures, pic->view);
	view.pictures[PICTURE_SLOT_NEXT] = view.pictures[PICTURE_SLOT_CURRENT];
	view.pictures[PICTURE_SLOT_CURRENT] = view.pictures[PICTURE_SLOT_PREV];
	if (pic->file) {
		free(pic->file);
		pic->file = NULL;
	}
	view.pictures[PICTURE_SLOT_PREV] = pic;
	ClearPictureView(pic);
	UI_OpenPictureView(iter->filepath);
	return 0;
}

static int OpenNextPicture(void)
{
	Picture pic;
	FileIterator iter = view.iterator;
	if (!iter || iter->index >= iter->length - 1) {
		return -1;
	}
	iter->next(iter);
	UpdateSwitchButtons();
	pic = view.pictures[PICTURE_SLOT_PREV];
	Widget_Append(view.view_pictures, pic->view);
	view.pictures[PICTURE_SLOT_PREV] = view.pictures[PICTURE_SLOT_CURRENT];
	view.pictures[PICTURE_SLOT_CURRENT] = view.pictures[PICTURE_SLOT_NEXT];
	if (pic->file) {
		free(pic->file);
		pic->file = NULL;
	}
	view.pictures[PICTURE_SLOT_NEXT] = pic;
	ClearPictureView(pic);
	UI_OpenPictureView(iter->filepath);
	return 0;
}

static void InitSlideTransition(void)
{
	struct SlideTransition *st;
	st = &view.slide;
	st->direction = 0;
	st->src_x = 0;
	st->dst_x = 0;
	st->timer = 0;
	st->duration = 200;
	st->start_time = 0;
	st->is_running = FALSE;
}

static void SetSliderPostion(int x)
{
	float fx = (float)x;
	float width = view.picture->view->width;
	Widget_Move(view.pictures[PICTURE_SLOT_PREV]->view, fx - width, 0);
	Widget_Move(view.pictures[PICTURE_SLOT_CURRENT]->view, fx, 0);
	Widget_Move(view.pictures[PICTURE_SLOT_NEXT]->view, fx + width, 0);
}

/** 更新滑动过渡效果 */
static void OnSlideTransition(void *arg)
{
	int delta, x;
	unsigned int  delta_time;
	struct SlideTransition *st;

	st = &view.slide;
	delta = st->dst_x - st->src_x;
	delta_time = (unsigned int)LCUI_GetTimeDelta(st->start_time);
	if (delta_time < st->duration) {
		x = st->src_x + delta * (int)delta_time / (int)st->duration;
	} else {
		x = st->dst_x;
	}
	SetSliderPostion(x);
	if (x == st->dst_x) {
		st->timer = 0;
		st->is_running = FALSE;
		ClearPositionStyle(view.pictures[PICTURE_SLOT_PREV]->view);
		ClearPositionStyle(view.pictures[PICTURE_SLOT_CURRENT]->view);
		ClearPositionStyle(view.pictures[PICTURE_SLOT_NEXT]->view);
		if (st->action != SWITCH) {
			return;
		}
		if (st->direction == RIGHT) {
			OpenPrevPicture();
		} else {
			OpenNextPicture();
		}
		return;
	}
	st->timer = LCUITimer_Set(10, OnSlideTransition, NULL, FALSE);
}

/** 还原滑动区块的位置，一般用于将拖动后的图片移动回原来的位置 */
static void RestoreSliderPosition(void)
{
	struct SlideTransition *st;
	st = &view.slide;
	if (st->is_running) {
		return;
	}
	st->action = RESTORE;
	st->src_x = (int)view.picture->view->x;
	if (st->src_x == 0) {
		return;
	} else if (st->src_x < 0) {
		st->direction = RIGHT;
	} else {
		st->direction = LEFT;
	}
	st->dst_x = 0;
	st->is_running = TRUE;
	st->start_time = LCUI_GetTime();
	if (st->timer > 0) {
		return;
	}
	st->timer = LCUITimer_Set(20, OnSlideTransition, NULL, FALSE);
}

static void StartSlideTransition(int direction)
{
	struct SlideTransition *st;
	st = &view.slide;
	if (st->is_running) {
		return;
	}
	st->action = SWITCH;
	st->direction = direction;
	st->src_x = (int)view.picture->view->x;
	if (st->direction == RIGHT) {
		st->dst_x = (int)view.picture->view->width;
	} else {
		st->dst_x = 0 - (int)view.picture->view->width;
	}
	st->is_running = TRUE;
	st->start_time = LCUI_GetTime();
	if (st->timer > 0) {
		return;
	}
	st->timer = LCUITimer_Set(20, OnSlideTransition, NULL, FALSE);
}

static int SwitchPrevPicture(void)
{
	FileIterator iter = view.iterator;
	if (!iter || iter->index == 0) {
		return -1;
	}
	StartSlideTransition(RIGHT);
	return 0;
}

static int SwitchNextPicture(void)
{
	FileIterator iter = view.iterator;
	if (!iter || iter->index >= iter->length - 1) {
		return -1;
	}
	StartSlideTransition(LEFT);
	return 0;
}

/** 更新图片位置 */
static void UpdatePicturePosition(Picture pic)
{
	LCUI_StyleSheet sheet;
	float x = 0, y = 0, width, height;
	sheet = pic->view->custom_style;
	width = (float)(pic->data->width * pic->scale);
	height = (float)(pic->data->height * pic->scale);
	if (width <= pic->view->width) {
		SetStyle(sheet, key_background_position_x, 0.5, scale);
	}
	if (height <= pic->view->height) {
		SetStyle(sheet, key_background_position_y, 0.5, scale);
	}
	if (pic != view.picture) {
		Widget_UpdateStyle(pic->view, FALSE);
		return;
	}
	/* 若缩放后的图片宽度小于图片查看器的宽度 */
	if (width <= pic->view->width) {
		/* 设置拖动时不需要改变X坐标，且图片水平居中显示 */
		view.drag.with_x = FALSE;
		view.focus_x = iround(width / 2.0);
		view.origin_focus_x = iround(pic->data->width / 2.0);
		SetStyle(sheet, key_background_position_x, 0.5, scale);
	} else {
		view.drag.with_x = TRUE;
		x = (float)view.origin_focus_x;
		view.focus_x = iround(x * pic->scale);
		x = (float)view.focus_x - view.offset_x;
		/* X坐标调整，确保查看器的图片浏览范围不超出图片 */
		if (x < 0) {
			x = 0;
			view.focus_x = view.offset_x;
		}
		if (x + pic->view->width > width) {
			x = width - pic->view->width;
			view.focus_x = (int)(x + view.offset_x);
		}
		_DEBUG_MSG("focus_x: %d\n", view.focus_x);
		SetStyle(sheet, key_background_position_x, -x, px);
		/* 根据缩放后的焦点坐标，计算出相对于原始尺寸图片的焦点坐标 */
		x = (float)(view.focus_x / pic->scale + 0.5);
		view.origin_focus_x = (int)x;
	}
	/* 原理同上 */
	if (height <= pic->view->height) {
		view.drag.with_y = FALSE;
		view.focus_y = iround(height / 2);
		view.origin_focus_y = iround(pic->data->height / 2.0);
		SetStyle(sheet, key_background_position_y, 0.5, scale);
	} else {
		view.drag.with_y = TRUE;
		y = (float)view.origin_focus_y;
		view.focus_y = (int)(y * pic->scale + 0.5);
		y = (float)(view.focus_y - view.offset_y);
		if (y < 0) {
			y = 0;
			view.focus_y = view.offset_y;
		}
		if (y + pic->view->height > height) {
			y = height - pic->view->height;
			view.focus_y = (int)(y + view.offset_y);
		}
		SetStyle(sheet, key_background_position_y, -y, px);
		y = (float)(view.focus_y / pic->scale + 0.5);
		view.origin_focus_y = (int)y;
	}
	Widget_UpdateStyle(pic->view, FALSE);
	PictureView_SetLabels();
}

/** 重置浏览区域的位置偏移量 */
static void ResetOffsetPosition(void)
{
	view.offset_x = iround(view.picture->view->width / 2);
	view.offset_y = iround(view.picture->view->height / 2);
	PictureView_SetLabels();
}

/** 设置当前图片缩放比例 */
static void DirectSetPictureScale(Picture pic, double scale)
{
	float width, height;
	LCUI_StyleSheet sheet;
	if (scale <= pic->min_scale) {
		scale = pic->min_scale;
	}
	if (scale > MAX_SCALE) {
		scale = MAX_SCALE;
	}
	pic->scale = scale;
	if (!pic->data) {
		return;
	}
	sheet = pic->view->custom_style;
	width = (float)(scale * pic->data->width);
	height = (float)(scale * pic->data->height);
	SetStyle(sheet, key_background_size_width, width, px);
	SetStyle(sheet, key_background_size_height, height, px);
	Widget_UpdateStyle(pic->view, FALSE);
	UpdatePicturePosition(pic);
	if (pic == view.picture) {
		UpdateZoomButtons();
		UpdateSwitchButtons();
		UpdateResetSizeButton();
		PictureView_SetLabels();
	}
}

static void SetPictureScale(Picture pic, double scale)
{
	if (scale <= pic->min_scale) {
		view.is_zoom_mode = FALSE;
	} else {
		view.is_zoom_mode = TRUE;
	}
	DirectSetPictureScale(pic, scale);
}

static void SetPictureZoomIn(Picture pic)
{
	SetPictureScale(pic, pic->scale * (1.0 + SCALE_STEP));
}

static void SetPictureZoomOut(Picture pic)
{
	SetPictureScale(pic, pic->scale * (1.0 - SCALE_STEP));
}

/** 重置当前显示的图片的尺寸 */
static void ResetPictureSize(Picture pic)
{
	size_t width, height;
	double scale_x, scale_y;
	if (!pic->data || !Graph_IsValid(pic->data)) {
		return;
	}
	GetViewerSize(&width, &height);
	/* 如果尺寸小于图片查看器尺寸 */
	if (pic->data->width < width && pic->data->height < height) {
		pic->scale = 1.0;
	} else {
		scale_x = 1.0 * width / pic->data->width;
		scale_y = 1.0 * height / pic->data->height;
		if (scale_y < scale_x) {
			pic->scale = scale_y;
		} else {
			pic->scale = scale_x;
		}
		if (pic->scale < 0.05) {
			pic->scale = 0.05;
		}
	}
	pic->min_scale = pic->scale;
	ResetOffsetPosition();
	DirectSetPictureScale(pic, pic->scale);
}

/** 在返回按钮被点击的时候 */
static void OnBtnBackClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	if (view.mode == MODE_SINGLE_PICVIEW) {
		LCUI_Widget btn_back, btn_back2;
		SelectWidget(btn_back, ID_BTN_BROWSE_ALL);
		SelectWidget(btn_back2, ID_BTN_HIDE_PICTURE_VIEWER);
		Widget_UnsetStyle(btn_back2, key_display);
		Widget_Destroy(btn_back);
		Widget_Show(btn_back2);
		view.mode = MODE_FULL;
		UI_InitMainView();
	}
	UI_ClosePictureView();
}

/** 在图片视图尺寸发生变化的时候 */
static void OnPictureResize(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	if (view.is_zoom_mode) {
		UpdatePicturePosition(view.pictures[PICTURE_SLOT_PREV]);
		UpdatePicturePosition(view.pictures[PICTURE_SLOT_CURRENT]);
		UpdatePicturePosition(view.pictures[PICTURE_SLOT_NEXT]);
	} else {
		ResetPictureSize(view.pictures[PICTURE_SLOT_PREV]);
		ResetPictureSize(view.pictures[PICTURE_SLOT_CURRENT]);
		ResetPictureSize(view.pictures[PICTURE_SLOT_NEXT]);
	}
}

static void OnShowTipLoading(void *arg)
{
	Picture pic = arg;
	if (view.picture == pic) {
		if (pic->is_loading) {
			ProgressBar_SetValue(view.tip_progress, 0);
			Widget_Show(view.tip_loading);
		}
	}
	pic->timer = 0;
}

static void StartGesture(int mouse_x, int mouse_y)
{
	struct Gesture *g;
	g = &view.gesture;
	g->x = mouse_x;
	g->y = mouse_y;
	g->start_x = mouse_x;
	g->start_y = mouse_y;
	g->timestamp = LCUI_GetTime();
	g->is_running = TRUE;
}

static void StopGesture(void)
{
	view.gesture.is_running = FALSE;
}

static void UpdateGesture(int mouse_x, int mouse_y)
{
	struct Gesture *g = &view.gesture;
	/* 只处理左右滑动 */
	if (g->x != mouse_x) {
		/* 如果滑动方向不同 */
		if ((g->x > g->start_x && mouse_x < g->x) ||
			(g->x < g->start_x && mouse_x > g->x)) {
			g->start_x = mouse_x;
		}
		g->x = mouse_x;
		g->timestamp = LCUI_GetTime();
	}
	g->y = mouse_y;
}

static int HandleGesture(void)
{
	struct Gesture *g = &view.gesture;
	int delta_time = (int)LCUI_GetTimeDelta(g->timestamp);
	/* 如果坐标停止变化已经超过 100ms，或滑动距离太短，则视为无效手势 */
	if (delta_time > 100 || abs(g->x - g->start_x) < 80) {
		return -1;
	}
	if (g->x > g->start_x) {
		return SwitchPrevPicture();
		return 0;
	} else if (g->x < g->start_x) {
		return SwitchNextPicture();
	}
	return -1;
}

static void DragPicture(int mouse_x, int mouse_y)
{
	LCUI_StyleSheet sheet;
	Picture pic = view.picture;
	if (!view.drag.is_running) {
		return;
	}
	sheet = pic->view->custom_style;
	if (view.drag.with_x) {
		float x, width;
		width = (float)(pic->data->width * pic->scale);
		view.focus_x = view.drag.focus_x;
		view.focus_x -= mouse_x - view.drag.mouse_x;
		x = (float)(view.focus_x - view.offset_x);
		if (x < 0) {
			x = 0;
			view.focus_x = view.offset_x;
		}
		if (x + pic->view->width > width) {
			x = width - pic->view->width;
			view.focus_x = iround(x + view.offset_x);
		}
		SetStyle(pic->view->custom_style,
			 key_background_position_x, -x, px);
		x = (float)(view.focus_x / pic->scale);
		view.origin_focus_x = iround(x);
	} else {
		SetStyle(sheet, key_background_position_x, 0.5, scale);
	}
	if (view.drag.with_y) {
		float y, height;
		height = (float)(pic->data->height * pic->scale);
		view.focus_y = view.drag.focus_y;
		view.focus_y -= mouse_y - view.drag.mouse_y;
		y = (float)(view.focus_y - view.offset_y);
		if (y < 0) {
			y = 0;
			view.focus_y = view.offset_y;
		}
		if (y + pic->view->height > height) {
			y = height - pic->view->height;
			view.focus_y = iround(y + view.offset_y);
		}
		SetStyle(pic->view->custom_style,
			 key_background_position_y, -y, px);
		y = (float)(view.focus_y / pic->scale);
		view.origin_focus_y = iround(y);
	} else {
		SetStyle(sheet, key_background_position_y, 0.5, scale);
	}
	Widget_UpdateStyle(pic->view, FALSE);
	PictureView_SetLabels();
}

/** 当鼠标在图片容器上移动的时候 */
static void OnPictureMouseMove(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	/* 如果是触控模式就不再处理鼠标事件了，因为触控事件中已经处理了一次图片拖动 */
	if (view.is_touch_mode) {
		return;
	}
	DragPicture(e->motion.x, e->motion.y);
}

/** 当鼠标按钮在图片容器上释放的时候 */
static void OnPictureMouseUp(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	if (view.is_touch_mode) {
		return;
	}
	view.drag.is_running = FALSE;
	Widget_UnbindEvent(w, "mouseup", OnPictureMouseUp);
	Widget_UnbindEvent(w, "mousemove", OnPictureMouseMove);
	Widget_ReleaseMouseCapture(w);
}

/** 当鼠标按钮在图片容器上点住的时候 */
static void OnPictureMouseDown(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	if (view.is_touch_mode) {
		return;
	}
	view.drag.is_running = TRUE;
	view.drag.mouse_x = e->motion.x;
	view.drag.mouse_y = e->motion.y;
	view.drag.focus_x = view.focus_x;
	view.drag.focus_y = view.focus_y;
	Widget_BindEvent(w, "mousemove", OnPictureMouseMove, NULL, NULL);
	Widget_BindEvent(w, "mouseup", OnPictureMouseUp, NULL, NULL);
	Widget_SetMouseCapture(w);
}

static void OnPictureTouchDown(LCUI_TouchPoint point)
{
	view.is_touch_mode = TRUE;
	view.drag.is_running = TRUE;
	if (!view.is_zoom_mode) {
		StartGesture(point->x, point->y);
	}
	view.drag.mouse_x = point->x;
	view.drag.mouse_y = point->y;
	view.drag.focus_x = view.focus_x;
	view.drag.focus_y = view.focus_y;
	Widget_SetTouchCapture(view.picture->view, point->id);
}

static void OnPictureTouchUp(LCUI_TouchPoint point)
{
	view.is_touch_mode = FALSE;
	view.drag.is_running = FALSE;
	if (view.gesture.is_running) {
		if (!view.zoom.is_running && !view.is_zoom_mode) {
			if (HandleGesture() != 0) {
				RestoreSliderPosition();
			}
		}
		StopGesture();
	}
	Widget_ReleaseTouchCapture(view.picture->view, -1);
}

static void OnPictureTouchMove(LCUI_TouchPoint point)
{
	DragPicture(point->x, point->y);
	if (view.gesture.is_running) {
		UpdateGesture(point->x, point->y);
		SetSliderPostion(point->x - view.drag.mouse_x);
	}
}

static void OnPictureTouch(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	double distance, scale;
	int i, x, y, *point_ids;
	LCUI_TouchPoint points[2] = { NULL, NULL };
	if (e->touch.n_points == 0) {
		return;
	}
	point_ids = view.zoom.point_ids;
	if (e->touch.n_points == 1) {
		LCUI_TouchPoint point;
		point = &e->touch.points[0];
		switch (point->state) {
		case LCUI_WEVENT_TOUCHDOWN:
			OnPictureTouchDown(point);
			point_ids[0] = point->id;
			break;
		case LCUI_WEVENT_TOUCHMOVE:
			OnPictureTouchMove(point);
			break;
		case LCUI_WEVENT_TOUCHUP:
			OnPictureTouchUp(point);
			point_ids[0] = -1;
			break;
		default: break;
		}
		return;
	}
	StopGesture();
	view.drag.is_running = FALSE;
	/* 遍历各个触点，确定两个用于缩放功能的触点 */
	for (i = 0; i < e->touch.n_points; ++i) {
		if (point_ids[0] == -1) {
			points[0] = &e->touch.points[i];
			point_ids[0] = points[0]->id;
			continue;
		} else if (point_ids[0] == e->touch.points[i].id) {
			points[0] = &e->touch.points[i];
			continue;
		}
		if (view.zoom.point_ids[1] == -1) {
			points[1] = &e->touch.points[i];
			point_ids[1] = points[1]->id;
		} else if (point_ids[1] == e->touch.points[i].id) {
			points[1] = &e->touch.points[i];
		}
	}
	/* 计算两触点的距离 */
	distance = pow(points[0]->x - points[1]->x, 2);
	distance += pow(points[0]->y - points[1]->y, 2);
	distance = sqrt(distance);
	if (!view.zoom.is_running) {
		x = points[0]->x - (points[0]->x - points[1]->x) / 2;
		y = points[0]->y - (points[0]->y - points[1]->y) / 2;
		view.zoom.distance = (int)distance;
		view.zoom.scale = view.picture->scale;
		view.zoom.is_running = TRUE;
		view.zoom.x = x;
		view.zoom.y = y;
	} else if (points[0]->state == LCUI_WEVENT_TOUCHMOVE ||
		   points[1]->state == LCUI_WEVENT_TOUCHMOVE) {
		/* 重新计算焦点位置 */
		x = view.focus_x - view.offset_x;
		y = view.focus_y - view.offset_y;
		view.offset_x = view.zoom.x;
		view.offset_y = view.zoom.y;
		view.focus_x = x + view.offset_x;
		view.focus_y = y + view.offset_y;
		x = iround(view.focus_x / view.picture->scale);
		y = iround(view.focus_y / view.picture->scale);
		view.origin_focus_x = x;
		view.origin_focus_y = y;
		scale = view.zoom.scale;
		scale *= distance / view.zoom.distance;
		SetPictureScale(view.picture, scale);
	}
	for (i = 0; i < 2; ++i) {
		int j, id;
		/* 如果当前用于控制缩放的触点已经释放，则找其它触点来代替 */
		if (points[i]->state != LCUI_WEVENT_TOUCHUP) {
			continue;
		}
		point_ids[i] = -1;
		view.zoom.is_running = FALSE;
		id = point_ids[i == 0 ? 1 : 0];
		for (j = 0; j < e->touch.n_points; ++j) {
			if (e->touch.points[j].state == LCUI_WEVENT_TOUCHUP ||
			    e->touch.points[j].id == id) {
				continue;
			}
			points[i] = &e->touch.points[j];
			point_ids[i] = points[i]->id;
			i -= 1;
			break;
		}
	}
	if (point_ids[0] == -1) {
		point_ids[0] = point_ids[1];
		point_ids[1] = -1;
		view.zoom.is_running = FALSE;
	} else if (point_ids[1] == -1) {
		view.zoom.is_running = FALSE;
	}
}

/** 初始化图片实例 */
static Picture CreatePicture(void)
{
	ASSIGN(pic, Picture);
	pic->timer = 0;
	pic->file = NULL;
	pic->is_valid = FALSE;
	pic->is_loading = FALSE;
	pic->file_for_load = NULL;
	pic->data = malloc(sizeof(LCUI_Graph));
	pic->view = LCUIWidget_New("picture");
	pic->min_scale = pic->scale = 1.0;

	Graph_Init(pic->data);
	Widget_Append(view.view_pictures, pic->view);
	BindEvent(pic->view, "resize", OnPictureResize);
	BindEvent(pic->view, "touch", OnPictureTouch);
	BindEvent(pic->view, "mousedown", OnPictureMouseDown);
	LCUIMutex_Init(&pic->mutex);
	LCUICond_Init(&pic->cond);
	return pic;
}

static void DeletePicture(Picture pic)
{
	if (!pic) {
		return;
	}
	LCUICond_Destroy(&pic->cond);
	LCUIMutex_Destroy(&pic->mutex);
	if (pic->data) {
		Graph_Free(pic->data);
		free(pic->data);
		pic->data = NULL;
	}
	if (pic->file_for_load) {
		free(pic->file_for_load);
		pic->file_for_load = NULL;
	}
	if (pic->file) {
		free(pic->file);
		pic->file = NULL;
	}
	pic->view = NULL;
	free(pic);
}

/** 设置图片 */
static void SetPicture(Picture pic, const wchar_t *file)
{
	int len;
	if (pic->timer > 0) {
		LCUITimer_Free(pic->timer);
		pic->timer = 0;
	}
	if (pic->file_for_load) {
		free(pic->file_for_load);
		pic->file_for_load = NULL;
	}
	if (file) {
		len = wcslen(file) + 1;
		pic->file_for_load = malloc(sizeof(wchar_t) * len);
		wcsncpy(pic->file_for_load, file, len);
	} else {
		pic->file_for_load = NULL;
	}
}

static void DirectSetPictureView(FileIterator iter)
{
	view.iterator = iter;
	UpdateSwitchButtons();
}

/** 设置图片预加载列表 */
static void SetPicturePreloadList(void)
{
	wchar_t *wpath;
	FileIterator iter;

	if (!view.iterator) {
		return;
	}
	iter = view.iterator;
	/* 预加载上一张图片 */
	if (iter->index > 0) {
		iter->prev(iter);
		if (iter->filepath) {
			wpath = DecodeUTF8(iter->filepath);
			SetPicture(view.pictures[PICTURE_SLOT_PREV], wpath);
		}
		iter->next(iter);
	}
	/* 预加载下一张图片 */
	if (iter->index < iter->length - 1) {
		iter->next(iter);
		if (iter->filepath) {
			wpath = DecodeUTF8(iter->filepath);
			SetPicture(view.pictures[PICTURE_SLOT_NEXT], wpath);
		}
		iter->prev(iter);
	}
}

static void OnPictureLoadDone(LCUI_Graph *img, void *data)
{
	Picture pic = data;
	LCUIMutex_Lock(&pic->mutex);
	if (img) {
		pic->data = malloc(sizeof(LCUI_Graph));
		*pic->data = *img;
		Graph_Init(img);
		pic->is_valid = TRUE;
	} else {
		pic->is_valid = FALSE;
	}
	pic->is_loading = FALSE;
	LCUICond_Signal(&pic->cond);
	LCUIMutex_Unlock(&pic->mutex);
}

static void OnPictureProgress(float progress, void *data)
{
	Picture pic = data;
	if (pic != view.picture) {
		return;
	}
	ProgressBar_SetValue(view.tip_progress, iround(progress));
}

static int LoadPictureAsync(Picture pic)
{
	wchar_t *wpath;
	int storage = finder.storage_for_image;

	if (!pic->file_for_load || !view.is_working) {
		return -1;
	}
	/* 避免重复加载 */
	if (pic->file && wcscmp(pic->file, pic->file_for_load) == 0) {
		free(pic->file_for_load);
		pic->file_for_load = NULL;
		return 0;
	}
	/** 300毫秒后显示 "图片载入中..." 的提示 */
	pic->timer = LCUITimer_Set(300, OnShowTipLoading, pic, FALSE);
	if (pic->data && Graph_IsValid(pic->data)) {
		ClearPictureView(pic);
	}
	wpath = pic->file_for_load;
	pic->file_for_load = NULL;
	_DEBUG_MSG("load: %ls\n", wpath);
	if (pic->file) {
		free(pic->file);
	}
	pic->file = wpath;
	pic->is_valid = FALSE;
	pic->is_loading = TRUE;
	/* 异步请求加载图像内容 */
	FileStorage_GetImage(storage, pic->file, OnPictureLoadDone,
			     OnPictureProgress, pic);
	return 0;
}

/** 等待图片加载完成 */
static void WaitPictureLoadDone(Picture pic)
{
	_DEBUG_MSG("enter\n");
	LCUIMutex_Lock(&pic->mutex);
	while (pic->is_loading && view.is_working) {
		LCUICond_TimedWait(&pic->cond, &pic->mutex, 1000);
	}
	LCUIMutex_Unlock(&pic->mutex);
	_DEBUG_MSG("exit\n");
	if (!view.is_working) {
		return;
	}
	/* 如果在加载完后没有待加载的图片，则直接呈现到部件上 */
	if (pic->is_valid && !pic->file_for_load) {
		SetPictureView(pic);
		ResetPictureSize(pic);
	}
	pic->is_loading = FALSE;
	if (view.picture->view == pic->view) {
		if (!pic->file_for_load) {
			Widget_Hide(view.tip_loading);
		}
		if (pic->is_valid) {
			Widget_Hide(view.tip_unsupport);
		} else {
			Widget_Show(view.tip_unsupport);
		}
		UpdateResetSizeButton();
		UpdateZoomButtons();
	}
}

/** 在”放大“按钮被点击的时候 */
static void OnBtnZoomInClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	double scale;
	Picture pic = view.picture;
	ResetOffsetPosition();
	if (!pic->data) {
		return;
	}
	scale = pic->scale * (1.0 + SCALE_STEP);
	SetPictureScale(view.picture, scale);
}

/** 在”缩小“按钮被点击的时候 */
static void OnBtnZoomOutClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	double scale;
	Picture pic = view.picture;
	if (!pic->data) {
		return;
	}
	ResetOffsetPosition();
	scale = pic->scale * (1.0 - SCALE_STEP);
	SetPictureScale(view.picture, scale);
}

static void OnBtnPrevClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	OpenPrevPicture();
}

static void OnBtnNextClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	OpenNextPicture();
}

static void OnBtnDeleteClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	char *path;
	wchar_t *wpath = view.picture->file;
	const wchar_t *title = I18n_GetText(KEY_DELETE);

	if (!LCUIDialog_Confirm(view.window, title, NULL)) {
		return;
	}
	MoveFileToTrashW(wpath);
	ClearPictureView(view.picture);
	path = EncodeUTF8(wpath);
	view.iterator->unlink(view.iterator);
	LCFinder_DeleteFiles(&path, 1, NULL, NULL);
	LCFinder_TriggerEvent(EVENT_FILE_DEL, path);
	free(path);

	if (view.iterator->length == 0) {
		LCUI_PostSimpleTask(TaskForHideTipEmpty, NULL, NULL);
		if (view.mode == MODE_SINGLE_PICVIEW) {
			LCUI_Quit();
		} else {
			UI_ClosePictureView();
		}
	} else {
		UI_OpenPictureView(view.iterator->filepath);
	}
}

/** 在”实际大小“按钮被点击的时候 */
static void OnBtnResetClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	double scale;
	if (view.picture->scale != view.picture->min_scale) {
		scale = view.picture->min_scale;
	} else {
		scale = 1.0;
	}
	ResetOffsetPosition();
	SetPictureScale(view.picture, scale);
}

/** 图片加载器线程 */
static void PictureLoader_Thread(void *arg)
{
	Picture pic;
	PictureLoader loader = arg;
	LCUIMutex_Lock(&loader->mutex);
	while (loader->is_running) {
		_DEBUG_MSG("wait load\n");
		LCUICond_Wait(&loader->cond, &loader->mutex);
		_DEBUG_MSG("start load\n");
		for (loader->num = 0; loader->num < 3 &&
		     loader->is_running; ++loader->num) {
			/* 图片实例组是这样记录图片的：
			 * [0] 上一张图片
			 * [1] 当前显示的图片
			 * [2] 下一张图片
			 * 而加载器是这样记录下标的：
			 * [0] 当前显示的图片
			 * [1] 下一张图片
			 * [2] 上一张图片
			 * 因此需要经过转换才能取到对应的图片实例
			 */
			switch (loader->num) {
			case 0:
				pic = loader->pictures[PICTURE_SLOT_CURRENT];
				break;
			case 1:
				pic = loader->pictures[PICTURE_SLOT_NEXT];
				break;
			case 2:
				pic = loader->pictures[PICTURE_SLOT_PREV];
				break;
			default: continue;
			}
			LoadPictureAsync(pic);
			LCUIMutex_Unlock(&loader->mutex);
			WaitPictureLoadDone(pic);
			LCUIMutex_Lock(&loader->mutex);
		}
	}
	LCUIMutex_Unlock(&loader->mutex);
	LCUIThread_Exit(NULL);
}

/* 设置图片加载器的目标 */
static void PictureLoader_SetTarget(PictureLoader loader,
				    const wchar_t *path)
{
	LCUIMutex_Lock(&loader->mutex);
	SetPicture(view.picture, path);
	SetPicturePreloadList();
	loader->num = -1;
	/* 如果当前图片实例正在加载图片资源，则直接显示提示 */
	if (view.picture->is_loading) {
		ProgressBar_SetValue(view.tip_progress, 0);
		Widget_Show(view.tip_loading);
	}
	LCUICond_Signal(&loader->cond);
	LCUIMutex_Unlock(&loader->mutex);
}

static void PictureLoader_Init(PictureLoader loader, Picture pictures[3])
{
	loader->num = -1;
	loader->is_running = TRUE;
	loader->pictures = pictures;
	LCUICond_Init(&loader->cond);
	LCUIMutex_Init(&loader->mutex);
	LCUIThread_Create(&loader->thread, PictureLoader_Thread, loader);
}

static void PictureLoader_Exit(PictureLoader loader)
{
	LCUIMutex_Lock(&loader->mutex);
	loader->is_running = FALSE;
	LCUICond_Signal(&loader->cond);
	LCUIMutex_Unlock(&loader->mutex);
	LCUIThread_Join(loader->thread, NULL);
	LCUICond_Destroy(&loader->cond);
	LCUIMutex_Destroy(&loader->mutex);
}

static void OnBtnShowInfoClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	PictureView_ShowInfo();
}

static void OnBtnShowLabelsClick(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	PictureView_ShowLabels();
}

static void SetPictureFocusPoint(Picture pic, int focus_x, int focus_y)
{
	int x, y;
	float width, height;
	/* 获取浏览区域的位置 */
	x = focus_x - view.offset_x;
	y = focus_y - view.offset_y;
	width = (float)(pic->scale * pic->data->width);
	height = (float)(pic->scale * pic->data->height);
	/* 如果缩放后的尺寸小于浏览区域，则调整偏移量 */
	if (width < pic->view->width) {
		view.offset_x += (int)((pic->view->width - width) / 2);
	}
	if (height < pic->view->height) {
		view.offset_y += (int)((pic->view->height - height) / 2);
	}
	/* 调整浏览区域坐标 */
	if (x < 0) {
		x = 0;
	}
	if (x + pic->view->width > width) {
		x = iround(width - pic->view->width);
	}
	if (y < 0) {
		y = 0;
	}
	if (y + pic->view->height > height) {
		y = iround(height - pic->view->height);
	}
	/* 更新焦点位置 */
	view.focus_x = x + view.offset_x;
	view.focus_y = y + view.offset_y;
	view.origin_focus_x = iround(view.focus_x / pic->scale);
	view.origin_focus_y = iround(view.focus_y / pic->scale);
	UpdatePicturePosition(pic);
}

static void OnMouseWheel(LCUI_Widget w, LCUI_WidgetEvent e, void *arg)
{
	int x, y;
	float width, height;
	double scale = view.picture->scale;
	Picture pic = view.picture;
	/* 获取当前浏览区域的位置 */
	x = view.focus_x - view.offset_x;
	y = view.focus_y - view.offset_y;
	/* 更新区域偏移量 */
	view.offset_x = e->wheel.x;
	view.offset_y = e->wheel.y;
	width = (float)(scale * pic->data->width);
	height = (float)(scale * pic->data->height);
	/* 如果缩放后的尺寸小于浏览区域，则调整偏移量 */
	if (width < pic->view->width) {
		view.offset_x += (int)((pic->view->width - width) / 2);
	}
	if (height < pic->view->height) {
		view.offset_y += (int)((pic->view->height - height) / 2);
	}
	/* 调整浏览区域坐标 */
	if (x < 0) {
		x = 0;
	}
	if (x + pic->view->width > width) {
		x = iround(width - pic->view->width);
	}
	if (y < 0) {
		y = 0;
	}
	if (y + pic->view->height > height) {
		y = iround(height - pic->view->height);
	}
	/* 更新焦点位置 */
	view.focus_x = x + view.offset_x;
	view.focus_y = y + view.offset_y;
	view.origin_focus_x = iround(view.focus_x / pic->scale);
	view.origin_focus_y = iround(view.focus_y / pic->scale);
	if (e->wheel.delta < 0) {
		SetPictureZoomOut(pic);
	} else {
		SetPictureZoomIn(pic);
	}
}

static void OnKeyDown(LCUI_SysEvent e, void *data)
{
	int focus_x = view.focus_x;
	int focus_y = view.focus_y;
	switch (e->key.code) {
	case LCUI_KEY_MINUS:
		SetPictureZoomOut(view.picture);
		break;
	case LCUI_KEY_EQUAL:
		SetPictureZoomIn(view.picture);
		break;
	case LCUI_KEY_LEFT:
		if (!view.is_zoom_mode) {
			OpenPrevPicture();
			break;
		}
		focus_x -= MOVE_STEP;
		SetPictureFocusPoint(view.picture, focus_x, focus_y);
		break;
	case LCUI_KEY_RIGHT:
		if (!view.is_zoom_mode) {
			OpenNextPicture();
			break;
		}
		focus_x += MOVE_STEP;
		SetPictureFocusPoint(view.picture, focus_x, focus_y);
		break;
	case LCUI_KEY_UP:
		if (!view.is_zoom_mode) {
			break;
		}
		focus_y -= MOVE_STEP;
		SetPictureFocusPoint(view.picture, focus_x, focus_y);
		break;
	case LCUI_KEY_DOWN:
		if (!view.is_zoom_mode) {
			break;
		}
		focus_y += MOVE_STEP;
		SetPictureFocusPoint(view.picture, focus_x, focus_y);
	default: break;
	}
}

void UI_InitPictureView(int mode)
{
	LCUI_Widget box, btn_back, btn_back2, btn_info, btn_label, btn_del;

	box = LCUIBuilder_LoadFile(FILE_PICTURE_VIEW);
	if (box) {
		Widget_Top(box);
		Widget_Unwrap(box);
	}
	view.mode = mode;
	view.is_working = TRUE;
	view.is_opening = FALSE;
	view.zoom.point_ids[0] = -1;
	view.zoom.point_ids[1] = -1;
	view.zoom.is_running = FALSE;
	SelectWidget(btn_back, ID_BTN_BROWSE_ALL);
	SelectWidget(btn_del, ID_BTN_DELETE_PICTURE);
	SelectWidget(btn_info, ID_BTN_SHOW_PICTURE_INFO);
	SelectWidget(btn_label, ID_BTN_SHOW_PICTURE_LABEL);
	SelectWidget(btn_back2, ID_BTN_HIDE_PICTURE_VIEWER);
	SelectWidget(view.btn_prev, ID_BTN_PCITURE_PREV);
	SelectWidget(view.btn_next, ID_BTN_PCITURE_NEXT);
	SelectWidget(view.btn_zoomin, ID_BTN_PICTURE_ZOOM_IN);
	SelectWidget(view.btn_zoomout, ID_BTN_PICTURE_ZOOM_OUT);
	SelectWidget(view.btn_reset, ID_BTN_PICTURE_RESET_SIZE);
	SelectWidget(view.window, ID_WINDOW_PCITURE_VIEWER);
	SelectWidget(view.tip_empty, ID_TIP_PICTURE_NOT_FOUND);
	SelectWidget(view.tip_loading, ID_TIP_PICTURE_LOADING);
	SelectWidget(view.tip_unsupport, ID_TIP_PICTURE_UNSUPPORT);
	SelectWidget(view.view_pictures, ID_VIEW_PICTURE_TARGET);
	BindEvent(btn_back, "click", OnBtnBackClick);
	BindEvent(btn_back2, "click", OnBtnBackClick);
	BindEvent(btn_del, "click", OnBtnDeleteClick);
	BindEvent(btn_info, "click", OnBtnShowInfoClick);
	BindEvent(btn_label, "click", OnBtnShowLabelsClick);
	BindEvent(view.btn_prev, "click", OnBtnPrevClick);
	BindEvent(view.btn_next, "click", OnBtnNextClick);
	BindEvent(view.btn_reset, "click", OnBtnResetClick);
	BindEvent(view.btn_zoomin, "click", OnBtnZoomInClick);
	BindEvent(view.btn_zoomout, "click", OnBtnZoomOutClick);
	BindEvent(view.view_pictures, "mousewheel", OnMouseWheel);
	BindEvent(view.view_pictures, "dblclick", OnMouseDblClick);
	LCUI_BindEvent(LCUI_KEYDOWN, OnKeyDown, NULL, NULL);
	InitSlideTransition();
	view.pictures[PICTURE_SLOT_PREV] = CreatePicture();
	view.pictures[PICTURE_SLOT_CURRENT] = CreatePicture();
	view.pictures[PICTURE_SLOT_NEXT] = CreatePicture();
	view.picture = view.pictures[PICTURE_SLOT_PREV];
	view.tip_progress = LCUIWidget_New("progress");
	ProgressBar_SetMaxValue(view.tip_progress, 100);
	PictureLoader_Init(&view.loader, view.pictures);
	Widget_Append(view.tip_loading, view.tip_progress);
	Widget_Hide(view.window);
	PictureView_InitInfo();
	PictureView_InitLabels();
	UpdateSwitchButtons();
	if (view.mode == MODE_SINGLE_PICVIEW) {
		view.scanner = PictureView_CreateScanner(finder.storage_for_scan);
		Widget_SetStyle(btn_back2, key_display, SV_NONE, style);
		Widget_Hide(btn_back2);
	} else {
		Widget_Destroy(btn_back);
	}
}

void UI_OpenPictureView(const char *filepath)
{
	wchar_t *wpath, title[256] = { 0 };
	LCUI_Widget main_window, root;

	root = LCUIWidget_GetRoot();
	wpath = DecodeUTF8(filepath);
	view.is_opening = TRUE;
	view.is_zoom_mode = FALSE;
	view.picture = view.pictures[PICTURE_SLOT_CURRENT];
	SelectWidget(main_window, ID_WINDOW_MAIN);
	swprintf(title, 255, L"%ls - "LCFINDER_NAME, wgetfilename(wpath));
	Widget_SetTitleW(root, title);
	Widget_Hide(view.tip_unsupport);
	Widget_Hide(view.tip_empty);
	PictureLoader_SetTarget(&view.loader, wpath);
	PictureView_SetInfo(filepath);
	PictureView_SetLabels();
	Widget_Show(view.window);
	Widget_Hide(main_window);
	if (view.mode == MODE_SINGLE_PICVIEW) {
		PictureView_OpenScanner(view.scanner, wpath,
					DirectSetPictureView,
					SetPicturePreloadList);
	}
	free(wpath);
}

void UI_SetPictureView(FileIterator iter)
{
	if (view.iterator) {
		view.iterator->destroy(view.iterator);
		view.iterator = NULL;
	}
	DirectSetPictureView(iter);
}

void UI_ClosePictureView(void)
{
	LCUI_Widget main_window;
	LCUI_Widget root = LCUIWidget_GetRoot();
	SelectWidget(main_window, ID_WINDOW_MAIN);
	Widget_SetTitleW(root, LCFINDER_NAME);
	ResetPictureSize(view.picture);
	Widget_Show(main_window);
	Widget_Hide(view.window);
	view.is_opening = FALSE;
	UI_SetPictureView(NULL);
}

void UI_FreePictureView(void)
{
	view.is_working = FALSE;
	if (view.mode == MODE_SINGLE_PICVIEW) {
		PictureView_CloseScanner(view.scanner);
	}
	PictureLoader_Exit(&view.loader);
	DeletePicture(view.pictures[PICTURE_SLOT_PREV]);
	DeletePicture(view.pictures[PICTURE_SLOT_CURRENT]);
	DeletePicture(view.pictures[PICTURE_SLOT_NEXT]);
	view.pictures[PICTURE_SLOT_PREV] = NULL;
	view.pictures[PICTURE_SLOT_CURRENT] = NULL;
	view.pictures[PICTURE_SLOT_NEXT] = NULL;
}
