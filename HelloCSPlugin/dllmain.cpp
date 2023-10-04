// dllmain.cpp : Definiert den Einstiegspunkt für die DLL-Anwendung.
#include "pch.h"
#include "DBG.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


#include "TriglavPlugInSDK/TriglavPlugInSDK.h"

#include <string>
TriglavPlugInInt ModuleInitialize(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	TriglavPlugInInt ret = kTriglavPlugInCallResultFailed;

	// 初期化レコード
	auto pModuleInitializeRecord = pluginServer->recordSuite.moduleInitializeRecord;
	// 文字列サービス (文字列を扱う関数を提供)
	auto pStringService = pluginServer->serviceSuite.stringService;

	if (pModuleInitializeRecord != NULL && pStringService != NULL) {
		// ホスト
		auto hostObject = pluginServer->hostObject;
		// ホストのバージョンを取得
		TriglavPlugInInt hostVersion;
		pModuleInitializeRecord->getHostVersionProc(&hostVersion, hostObject);

		// ホストのバージョンが、このプラグインが必要としているバージョン以上なら登録処理
		if (hostVersion >= kTriglavPlugInNeedHostVersion) {
			// モジュール初期化レコードに moduleID を設定。
			TriglavPlugInStringObject	moduleID = NULL;
			// 各プラグインで唯一でなければならず、重複しないようにする
			std::string moduleIDString = "00001111-2222-3333-4444-555566667777";

			pStringService->createWithAsciiStringProc(&moduleID, moduleIDString.c_str(), moduleIDString.size());
			pModuleInitializeRecord->setModuleIDProc(hostObject, moduleID);
			// 初期化レコードにプラグインの種類を設定 (フィルタのプラグイン = kTriglavPlugInModuleSwitchKindFilter)
			pModuleInitializeRecord->setModuleKindProc(hostObject, kTriglavPlugInModuleSwitchKindFilter);
			pStringService->releaseProc(moduleID);

			// 登録成功
			ret = kTriglavPlugInCallResultSuccess;
			LOG("SUCCESS: %s", __func__)
		}
	}

	return ret;
}

TriglavPlugInInt ModuleTerminate(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	TriglavPlugInInt ret = kTriglavPlugInCallResultFailed;
	return kTriglavPlugInCallResultSuccess;
}

TriglavPlugInInt FilterTerminate(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	TriglavPlugInInt ret = kTriglavPlugInCallResultFailed;
	return kTriglavPlugInCallResultSuccess;
}


#include "resource.h"
#include <vector>
// メニュー、ダイアログの表示設定
TriglavPlugInInt FilterInitialize(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	auto pRecordSuite = &pluginServer->recordSuite;
	auto hostObject = pluginServer->hostObject;
	auto pStringService = pluginServer->serviceSuite.stringService;
	auto PropertyService = pluginServer->serviceSuite.propertyService;

	if (TriglavPlugInGetFilterInitializeRecord(pRecordSuite) == NULL || pStringService == NULL || PropertyService == NULL)
		return kTriglavPlugInCallResultFailed;

	TriglavPlugInStringObject	filterCategoryName = NULL;
	TriglavPlugInStringObject	filterName = NULL;
	// リソースで定義したIDから文字列を取得 (like MAKEINTRESOURCE)
	// TriglavPlugInStringObjectは内容に直接アクセスできないのでこの方法しかない
	pStringService->createWithStringIDProc(&filterCategoryName, ID_FilterCategoryName, pluginServer->hostObject);
	pStringService->createWithStringIDProc(&filterName, ID_FilterName, pluginServer->hostObject);

	// 初期化レコードにカテゴリ名, フィルタ名を設定
	// カテゴリ名のアクセスキーに [c]
	TriglavPlugInFilterInitializeSetFilterCategoryName(pRecordSuite, hostObject, filterCategoryName, 'c');
	// フィルタ名のアクセスキーに [h]
	TriglavPlugInFilterInitializeSetFilterName(pRecordSuite, hostObject, filterName, 'h');
	// 開放
	pStringService->releaseProc(filterCategoryName);
	pStringService->releaseProc(filterName);

	//	プレビュー checkbox を配置
	TriglavPlugInFilterInitializeSetCanPreview(pRecordSuite, hostObject, true);

	// カラーのラスターレイヤーのみを対象とする
	// それ以外のレイヤーは実行できないようにする
	std::vector<TriglavPlugInInt> target = { kTriglavPlugInFilterTargetKindRasterLayerRGBAlpha };
	TriglavPlugInFilterInitializeSetTargetKinds(pRecordSuite, hostObject, &target[0], target.size());

	return kTriglavPlugInCallResultSuccess;
}


#define rep(i,m) for(long i=0;i<m;i++)
TriglavPlugInInt FilterRun(TriglavPlugInPtr* data, TriglavPlugInServer* pluginServer) {
	TriglavPlugInInt ret = kTriglavPlugInCallResultFailed;

	auto pRecordSuite = &pluginServer->recordSuite;
	auto pOffscreenService = pluginServer->serviceSuite.offscreenService;
	auto pPropertyService = pluginServer->serviceSuite.propertyService;
	auto pBitmapService = pluginServer->serviceSuite.bitmapService;

	if (TriglavPlugInGetFilterRunRecord(pRecordSuite) == NULL || pOffscreenService == NULL || pPropertyService == NULL)
		return ret;

	const auto hostObject = pluginServer->hostObject;

	TriglavPlugInPropertyObject		propertyObject;
	TriglavPlugInOffscreenObject	sourceOffscreenObject;		// 編集対象; コピー元
	TriglavPlugInOffscreenObject	destinationOffscreenObject;	// 編集対象; コピー先
	TriglavPlugInRect				selectAreaRect;
	TriglavPlugInOffscreenObject	selectAreaOffscreenObject;
	TriglavPlugInFilterRunGetProperty(pRecordSuite, &propertyObject, hostObject);
	TriglavPlugInFilterRunGetSourceOffscreen(pRecordSuite, &sourceOffscreenObject, hostObject);
	TriglavPlugInFilterRunGetDestinationOffscreen(pRecordSuite, &destinationOffscreenObject, hostObject);
	TriglavPlugInFilterRunGetSelectAreaRect(pRecordSuite, &selectAreaRect, hostObject);
	TriglavPlugInFilterRunGetSelectAreaOffscreen(pRecordSuite, &selectAreaOffscreenObject, hostObject);

	// 編集対象の矩形領域から、、高さ、左上座標xyを取得
	TriglavPlugInInt width, height, top, left;
	if (selectAreaOffscreenObject != NULL) {
		top = selectAreaRect.top;
		left = selectAreaRect.left;
		width = abs(selectAreaRect.left - selectAreaRect.right);
		height = abs(selectAreaRect.top - selectAreaRect.bottom);
	}
	else {
		// 選択範囲なしのときは全キャンパス領域
		top = 0;
		left = 0;
		pOffscreenService->getWidthProc(&width, destinationOffscreenObject);
		pOffscreenService->getHeightProc(&height, destinationOffscreenObject);
	}

	// 選択範囲矩形と同じ大きさの画像用メモリを確保
	// これで画像処理を行う
	TriglavPlugInBitmapObject sourceBitmapObject = NULL;
	pBitmapService->createProc(&sourceBitmapObject, width, height, 3, kTriglavPlugInBitmapScanlineHorizontalLeftTop);

	// オフスクリーン (編集対象; source)　から画像用メモリへコピー 
	TriglavPlugInPoint bitmapSourcePoint{ 0, 0 };
	TriglavPlugInPoint offscreenSourcePoint{ left, top };
	pOffscreenService->getBitmapProc(sourceBitmapObject, &bitmapSourcePoint, sourceOffscreenObject, &offscreenSourcePoint, width, height, kTriglavPlugInOffscreenCopyModeImage);
	
	// (R,G,B) のインデックスを取得。(2,1,0)となったが、環境によっては変わるのだろう
	// ピクセルは4バイトであり、8bit値4つの配列で表されるので、その配列の位置を示している。
	TriglavPlugInInt rIndex, gIndex, bIndex;
	pOffscreenService->getRGBChannelIndexProc(&rIndex, &gIndex, &bIndex, sourceOffscreenObject);

	// 処理の進捗をホスト側に通知。ここでは、y座標1つにつき1進めるようにした。y=height-1で100%になる
	TriglavPlugInFilterRunSetProgressTotal(pRecordSuite, hostObject, height - 1);


	for (bool restart = true; true;) {
		// 最初の初期化
		// パラメータを設定する／し直す
		// プレビュー有効時、ダイアログでパラメータを変更したりすると、処理を中断して、新たなパラメータを設定して処理再開
		if (restart) {
			restart = false;

			// フィルタ処理を続行するか問い合わせ
			TriglavPlugInInt processResult;
			TriglavPlugInFilterRunProcess(pRecordSuite, &processResult, hostObject, kTriglavPlugInFilterRunProcessStateStart);
			// 終了要求なら抜ける
			if (processResult == kTriglavPlugInFilterRunProcessResultExit) break;
		}

		TriglavPlugInInt rowBytes, pixelBytes, depth;
		pBitmapService->getRowBytesProc(&rowBytes, sourceBitmapObject);
		pBitmapService->getPixelBytesProc(&pixelBytes, sourceBitmapObject);
		pBitmapService->getDepthProc(&depth, sourceBitmapObject);

		rep(y, height) {
			// 進捗を進める
			TriglavPlugInFilterRunSetProgressDone(pRecordSuite, hostObject, y);

			rep(x, width) {
				//TriglavPlugInPtr address;
				BYTE* address;
				TriglavPlugInPoint point = { x, y };
				pBitmapService->getAddressProc((TriglavPlugInPtr*)&address, sourceBitmapObject, &point);
				auto src = (BYTE*)address;
				// RGB 反転
				src[rIndex] = ~src[rIndex];
				src[gIndex] = ~src[gIndex];
				src[bIndex] = ~src[bIndex];
			}
		}

		// 編集した画像データをオフスクリーン (編集対象; destination)
		// オフスクリーンとは、レイヤーの描画データ
		pOffscreenService->setBitmapProc(destinationOffscreenObject, &offscreenSourcePoint, sourceBitmapObject, &bitmapSourcePoint, width, height, kTriglavPlugInOffscreenCopyModeImage);
		if (selectAreaOffscreenObject == NULL) {
			const TriglavPlugInRect rect{ 0, 0, width, height };
			TriglavPlugInFilterRunUpdateDestinationOffscreenRect(pRecordSuite, hostObject, &rect);
		}
		else {
			TriglavPlugInFilterRunUpdateDestinationOffscreenRect(pRecordSuite, hostObject, &selectAreaRect);
		}

		TriglavPlugInInt result;
		TriglavPlugInFilterRunProcess(pRecordSuite, &result, hostObject, kTriglavPlugInFilterRunProcessStateEnd);

		// フィルタの処理をやりなおす。変数 restart=true、パラメータを再設定
		if (result == kTriglavPlugInFilterRunProcessResultRestart)
			restart = true;

		// フィルタを終了
		else if (result == kTriglavPlugInFilterRunProcessResultExit)
			break;
	}

	ret = kTriglavPlugInCallResultSuccess;
	return ret;
}

void TRIGLAV_PLUGIN_API TriglavPluginCall(TriglavPlugInInt* result, TriglavPlugInPtr* data, TriglavPlugInInt selector, TriglavPlugInServer* pluginServer, TriglavPlugInPtr reserved) {
	*result = kTriglavPlugInCallResultFailed;

	if (pluginServer == NULL) return;

	switch (selector) {
	case kTriglavPlugInSelectorModuleInitialize:
		// プラグインモジュールの初期化
		*result = ModuleInitialize(data, pluginServer);
		break;

	case kTriglavPlugInSelectorModuleTerminate:
		//	プラグインの終了処理
		*result = ModuleTerminate(data, pluginServer);
		break;

	case kTriglavPlugInSelectorFilterTerminate:
		*result = FilterTerminate(data, pluginServer);
		//	フィルタの終了処理
		break;

	case kTriglavPlugInSelectorFilterInitialize:
		*result = FilterInitialize(data, pluginServer);
		// フィルタの初期化
		break;

	case kTriglavPlugInSelectorFilterRun:
		// フィルタの計算処理 (本体)
		*result = FilterRun(data, pluginServer);
		break;
	}
}