﻿/* ***************************************************************************
 * ui.h -- ui managment module
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
 * ui.h -- 图形界面管理模块
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

#ifndef LCFINDER_UI_H
#define LCFINDER_UI_H

#include "types.h"

/* 一些元素的 ID，命名格式为：ID_类型_名称 */
#define ID_WINDOW_MAIN			"main-window"
#define ID_WINDOW_PCITURE_VIEWER	"picture-viewer-window"
#define ID_PANEL_PICTURE_INFO		"picture-info-panel"
#define ID_TXT_HOME_SELECTION_STATS	"view-home-selection-stats"
#define ID_TXT_SEARCH_SELECTION_STATS	"view-search-selection-stats"
#define ID_TXT_FOLDERS_SELECTION_STATS	"view-folders-selection-stats"
#define ID_TXT_PICTURE_NAME		"picture-info-name"
#define ID_TXT_PICTURE_TIME		"picture-info-time"
#define ID_TXT_PICTURE_FILE_SIZE	"picture-info-file-size"
#define ID_TXT_PICTURE_SIZE		"picture-info-size"
#define ID_TXT_PICTURE_PATH		"picture-info-path"
#define ID_TXT_FILE_SYNC_STATS		"file-sync-tip-stats"
#define ID_TXT_FILE_SYNC_TITLE		"file-sync-tip-title"
#define ID_TXT_THUMB_DB_SIZE		"text-thumb-db-size"
#define ID_TXT_CURRENT_LANGUAGE		"txt-current-language"
#define ID_TXT_CURRENT_SCALING		"txt-current-scaling"
#define ID_TXT_TRIAL_LICENSE		"txt-trial-license"
#define ID_VIEW_PICTURE_TAGS		"picture-info-tags"
#define ID_VIEW_PICTURE_TARGET		"picture-viewer-target"
#define ID_VIEW_PCITURE_RATING		"picture-info-rating"
#define ID_VIEW_MAIN_SIDEBAR		"main-sidebar"
#define ID_VIEW_FILE_LIST		"current-file-list"
#define ID_VIEW_TIME_RANGE_LIST		"time-range-list"
#define ID_VIEW_TIME_RANGE		"view-time-range"
#define ID_VIEW_HOME			"view-home"
#define ID_VIEW_HOME_COLLECTIONS	"home-collection-list"
#define ID_VIEW_FOLDERS			"view-folders"
#define ID_VIEW_FOLDER_INFO		"view-folders-info-box"
#define ID_VIEW_FOLDER_INFO_NAME	"view-folders-info-box-name"
#define ID_VIEW_FOLDER_INFO_PATH	"view-folders-info-box-path"
#define ID_VIEW_SOURCE_LIST		"current-source-list"
#define ID_VIEW_PRIVATE_SOURCE_LIST	"current-private-source-list"
#define ID_VIEW_PRIVATE_SPACE		"view-private-space"
#define ID_VIEW_SEARCH_TAGS		"view-search-tags"
#define ID_VIEW_SEARCH_TAGS_WRAPPER	"view-search-tags-wrapper"
#define ID_VIEW_SEARCH_RESULTS_WRAPPER	"view-search-results-wrapper"
#define ID_VIEW_SEARCH			"view-search"
#define ID_VIEW_SEARCH_FILES		"view-search-files"
#define ID_VIEW_HOME_PROGRESS		"view-home-progress"
#define ID_BTN_SEARCH_FILES		"btn-search-files"
#define ID_BTN_CLEAR_SEARCH		"btn-clear-search"
#define ID_BTN_SYNC_HOME_FILES		"btn-sync-home-files"
#define ID_BTN_TAG_HOME_FILES		"btn-tag-home-files"
#define ID_BTN_DELETE_HOME_FILES	"btn-delete-home-files"
#define ID_BTN_SELECT_HOME_FILES	"btn-select-home-files"
#define ID_BTN_CANCEL_HOME_SELECT	"btn-cancel-home-select"
#define ID_BTN_TAG_SEARCH_FILES		"btn-tag-search-files"
#define ID_BTN_DELETE_SEARCH_FILES	"btn-delete-search-files"
#define ID_BTN_SELECT_SEARCH_FILES	"btn-select-search-files"
#define ID_BTN_CANCEL_SEARCH_SELECT	"btn-cancel-search-select"
#define ID_BTN_TAG_FOLDER_FILES		"btn-tag-folder-files"
#define ID_BTN_DELETE_FOLDER_FILES	"btn-delete-folder-files"
#define ID_BTN_SELECT_FOLDER_FILES	"btn-select-folder-files"
#define ID_BTN_CANCEL_FOLDER_SELECT	"btn-cancel-folder-select"
#define ID_BTN_RETURN_ROOT_FOLDER	"btn-return-root-folder"
#define ID_BTN_SYNC_FOLDER_FILES	"btn-sync-folder-files"
#define ID_BTN_ADD_PICTURE_TAG		"btn-add-picture-tag"
#define ID_BTN_OPEN_PICTURE_DIR		"btn-open-picture-dir"
#define ID_BTN_HIDE_PICTURE_INFO	"btn-hide-picture-info"
#define ID_BTN_SHOW_PICTURE_INFO	"btn-show-picture-info"
#define ID_BTN_DELETE_PICTURE		"btn-delete-picture"
#define ID_BTN_HIDE_PICTURE_VIEWER	"btn-hide-picture-viewer"
#define ID_BTN_BROWSE_ALL		"btn-browse-all"
#define ID_BTN_PICTURE_RESET_SIZE	"btn-picture-reset-size"
#define ID_BTN_PICTURE_ZOOM_IN		"btn-picture-zoom-in"
#define ID_BTN_PICTURE_ZOOM_OUT		"btn-picture-zoom-out"
#define ID_BTN_PCITURE_PREV		"btn-picture-prev"
#define ID_BTN_PCITURE_NEXT		"btn-picture-next"
#define ID_BTN_ADD_SOURCE		"btn-add-source"
#define ID_BTN_ADD_PRIVATE_SOURCE	"btn-add-private-source"
#define ID_BTN_SIDEBAR_SETTINGS		"sidebar-btn-settings"
#define ID_BTN_SIDEBAR_SEARCH		"sidebar-btn-search"
#define ID_BTN_CLEAR_THUMB_DB		"btn-clear-thumb-db"
#define ID_BTN_HIDE_SEARCH_RESULT	"btn-hide-search-result"
#define ID_BTN_OPEN_PICTURE_DIR		"btn-open-picture-dir"
#define ID_BTN_RESET_PASSWORD		"btn-reset-password"
#define ID_TIP_HOME_EMPTY		"tip-empty-collection"
#define ID_TIP_FOLDERS_EMPTY		"tip-empty-folder"
#define ID_TIP_SEARCH_TAGS_EMPTY	"tip-search-tags-empty"
#define ID_TIP_SEARCH_FILES_EMPTY	"tip-search-no-result"
#define ID_TIP_PICTURE_LOADING		"tip-picture-loading"
#define ID_TIP_PICTURE_NOT_FOUND	"tip-picture-not-found"
#define ID_TIP_PICTURE_UNSUPPORT	"tip-picture-unsupport"
#define ID_INPUT_SEARCH			"input-search"
#define ID_SEARCH_ACTIONS		"view-search-actions"
#define ID_DROPDOWN_LANGUAGES		"dropdown-languages"
#define ID_DROPDOWN_FOLDER_FILES_SORT	"dropdown-folder-files-sort"
#define ID_DROPDOWN_SEARCH_FILES_SORT	"dropdown-search-files-sort"
#define ID_DROPDOWN_SCALING		"dropdown-scaling"
#define ID_SWITCH_PRIVATE_SPACE		"switch-private-space-open"

/* xml 文件位置 */
#define FILE_MAIN_VIEW		"assets/views/main.xml"
#define FILE_PICTURE_VIEW	"assets/views/picture.xml"

#define SelectWidget(V, ID) V = LCUIWidget_GetById( ID )
#define BindEvent(W, E, CB) Widget_BindEvent( W, E, CB, NULL, NULL )

enum UIMode {
	MODE_FULL,
	MODE_SINGLE_PICVIEW
};

/** 初始化主界面 */
void UI_InitMainView( void );

/** 初始化用户界面 */
int UI_Init( int argc, char **argv );

void UI_Free( void );

/** 让用户界面开始工作 */
int UI_Run( void );

/** 初始化侧边栏 */
void UI_InitSidebar( void );

/** 初始化“设置”视图 */
void UI_InitSettingsView( void );

/** 初始化“文件夹”视图 */
void UI_InitFoldersView( void );

void UI_FreeFoldersView( void );

/** 初始化文件同步时的提示框 */
void UI_InitFileSyncTip( void );

/** 初始化首页集锦视图 */
void UI_InitHomeView( void );

void UI_FreeHomeView( void );

/** 初始化图片视图 */
void UI_InitPictureView( int mode );

/** 退出图片视图并销毁相关资源 */
void UI_FreePictureView( void );

/** 在图片视图中打开一张图片 */
void UI_OpenPictureView( const char *filepath );

/**
 * 为图片视图设置相关数据
 * @param[in] iter 文件迭代器，用于图片的上一张/下一张的切换功能
 */
void UI_SetPictureView( FileIterator iter );

/** 关闭图片视图 */
void UI_ClosePictureView( void );

/** 初始化图片信息视图 */
void UI_InitPictureInfoView( void );

/** 设置图片信息视图 */
void UI_SetPictureInfoView( const char *filepath );

/** 显示图片信息视图 */
void UI_ShowPictureInfoView( void );

/** 隐藏图片信息视图 */
void UI_HidePictureInfoView( void );

void UI_UpdateSearchView( void );

void UI_InitSearchView( void );

/** 初始化启动画面 */
void UI_InitSplashScreen( void );

#endif
