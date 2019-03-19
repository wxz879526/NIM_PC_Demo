#include "login_form.h"
#include "gui/main/main_form.h"
#include "module/db/public_db.h"
#include "module/login/login_manager.h"
#include "guiex/main/main_form_ex.h"
#include "gui/main/main_form_menu.h"
#include "gui/plugins/chatroom_plugin.h"
#include "module/plugins/main_plugins_manager.h"
#include "gui/plugins/cef_test_plugin.h"
#include "gui/plugins/gif_test_plugin.h"
#include "module/config/config_helper.h"
#include "gui/plugins/addresbook_plugin/addresbook_plugin.h"
#include "shared/tool.h"
using namespace ui;

void LoginForm::OnLogin()
{
	DoInitUiKit(nim_ui::InitManager::kIM);
	SetAnonymousChatroomVisible(false);
	DoBeforeLogin();
}

void LoginForm::DoInitUiKit(nim_ui::InitManager::InitMode mode)
{
	// InitUiKit�ӿڵ�һ�����������Ƿ������¼�����ģ�飬Ĭ��Ϊfalse�����������demo app��Ϊtrue
	// ������App�������¼����Ĺ��ܣ���˲�����Ϊtrue
	nim_ui::InitManager::GetInstance()->InitUiKit(app_sdk::AppSDKInterface::IsNimDemoAppKey(app_sdk::AppSDKInterface::GetAppKey()), mode);
}
void LoginForm::DoBeforeLogin()
{
	std::string username = user_name_edit_->GetUTF8Text();
	StringHelper::Trim( username );
	std::wstring user_name_temp = nbase::UTF8ToUTF16(username);
	user_name_temp = StringHelper::MakeLowerString(user_name_temp);
	username = nbase::UTF16ToUTF8(user_name_temp);
	if( username.empty() )
	{
		usericon_->SetEnabled(false);
		ShowLoginTip(MutiLanSupport::GetInstance()->GetStringViaID(L"STRID_LOGIN_FORM_TIP_ACCOUNT_EMPTY"));
		return;
	}

	std::string password = password_edit_->GetUTF8Text();
	StringHelper::Trim( password );
	if( password.empty() )
	{
		passwordicon_->SetEnabled(false);
		ShowLoginTip(MutiLanSupport::GetInstance()->GetStringViaID(L"STRID_LOGIN_FORM_TIP_PASSWORD_EMPTY"));
		return;
	}

	usericon_->SetEnabled(true);
	passwordicon_->SetEnabled(true);

	StartLogin(username, password);
}

void LoginForm::DoRegisterAccount()
{
	MutiLanSupport* mls = MutiLanSupport::GetInstance();
	std::string username = user_name_edit_->GetUTF8Text();
	StringHelper::Trim(username);
	std::string password = password_edit_->GetUTF8Text();
	StringHelper::Trim(password);
	std::string nickname = nick_name_edit_->GetUTF8Text();
	StringHelper::Trim(nickname);
	if (password.length() < 6 || password.length() > 128)
	{
		ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_PASSWORD_RESTRICT"));
	}
	else if (nickname.empty())
	{
		ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_NICKNAME_RESTRICT"));
	}
	else 
	{
		btn_register_->SetEnabled(false);
		btn_login_->SetVisible(false);

		password = QString::GetMd5(password);
		auto task = ToWeakCallback([this](int res, const std::string& err_msg) {
			if (res == 200)
			{
				register_ok_toast_->SetVisible(true);
				nbase::ThreadManager::PostDelayedTask(ToWeakCallback([this]() {
					register_ok_toast_->SetVisible(false);
				}), nbase::TimeDelta::FromSeconds(2));

				nbase::ThreadManager::PostDelayedTask(ToWeakCallback([this]() {
					SetTaskbarTitle(MutiLanSupport::GetInstance()->GetStringViaID(L"STRID_LOGIN_FORM_LOGIN"));
					FindControl(L"enter_panel")->SetBkImage(L"user_password.png");
					FindControl(L"nick_name_panel")->SetVisible(false);
					FindControl(L"enter_login")->SetVisible(false);
					FindControl(L"register_account")->SetVisible(true);
					FindControl(L"login_cache_conf")->SetVisible(true);
					SetAnonymousChatroomVisible(!login_function_);
					btn_register_->SetEnabled(true);
					btn_register_->SetVisible(false);
					btn_login_->SetVisible(true);
				}), nbase::TimeDelta::FromMilliseconds(2500));
			}
			else
			{
				MutiLanSupport* mls = MutiLanSupport::GetInstance();
				if (res == 601) {
					ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_ACCOUNT_RESTRICT"));
				}
				else if (res == 602) {
					ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_ACCOUNT_EXIST"));
				}
				else if (res == 603) {
					ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_NICKNAME_TOO_LONG"));
				}
				else {
					ShowLoginTip(nbase::UTF8ToUTF16(err_msg));
				}
				btn_register_->SetEnabled(true);
			}
		});
		app_sdk::AppSDKInterface::GetInstance()->InvokeRegisterAccount(username, password, nickname, task);
	}
}

void LoginForm::StartLogin( std::string username, std::string password )
{
	if (!nim_ui::LoginManager::GetInstance()->CheckSingletonRun(nbase::UTF8ToUTF16(username)))
	{
		ShowMsgBox(this->GetHWND(), MsgboxCallback(), L"STRID_CHECK_SINGLETON_RUN", true);
		return;
	}
		
	user_name_edit_->SetEnabled(false);
	password_edit_->SetEnabled(false);

	login_error_tip_->SetVisible(false);
	login_ing_tip_->SetVisible(true);

	btn_login_->SetVisible(false);
	btn_cancel_->SetVisible(true);

	nim_ui::LoginManager::GetInstance()->DoLogin(username, password);
}

void LoginForm::RegLoginManagerCallback()
{
	nim_ui::OnLoginError cb_result = [this](int error){
		this->OnLoginError(error);
	};

	nim_ui::OnCancelLogin cb_cancel = [this]{
		this->OnCancelLogin();
	};

	nim_ui::OnHideWindow cb_hide = [this]{
		this->ShowWindow(false, false);
	};

	nim_ui::OnDestroyWindow cb_destroy = [this]{
		PublicDB::GetInstance()->SaveLoginData();
		::DestroyWindow(this->GetHWND());
	};

	nim_ui::OnShowMainWindow cb_show_main = [this]{
		nim_ui::UIStyle uistyle = nim_ui::UIStyle::join;
		switch (ConfigHelper::GetInstance()->GetUIStyle())
		{
		case  0:
			uistyle = nim_ui::UIStyle::conventional;
			break;
		case 1:
			uistyle = nim_ui::UIStyle::join;
			break;
		default:
			break;
		}

		nim_ui::RunTimeDataManager::GetInstance()->SetUIStyle(uistyle);
		switch (nim_ui::RunTimeDataManager::GetInstance()->GetUIStyle())
		{
		case nim_ui::UIStyle::join:
			nim_comp::MainPluginsManager::GetInstance()->RegPlugin<CefTestPlugin>("CefTestPlugin");
			nim_comp::MainPluginsManager::GetInstance()->RegPlugin<ChatroomPlugin>("ChatroomPlugin");
			nim_comp::MainPluginsManager::GetInstance()->RegPlugin<GifTestPlugin>("GifTestPlugin");
			nim_comp::MainPluginsManager::GetInstance()->RegPlugin<AddresBookPlugin>("AddresBookPlugin");
			nim_ui::WindowsManager::SingletonShow<nim_comp::MainFormEx>(nim_comp::MainFormEx::kClassName,new MainFormMenu());
			break;
		case nim_ui::UIStyle::conventional:
			nim_ui::WindowsManager::SingletonShow<MainForm>(MainForm::kClassName);
			break;
		default:
			break;
		}	
	};

	nim_ui::LoginManager::GetInstance()->RegLoginManagerCallback(ToWeakCallback(cb_result),
		ToWeakCallback(cb_cancel),
		ToWeakCallback(cb_hide),
		ToWeakCallback(cb_destroy),
		ToWeakCallback(cb_show_main));
}

void LoginForm::OnLoginError( int error )
{
	if (error == nim::kNIMResSuccess)
	{
		OnLoginOK();
	}
	else
	{
		OnCancelLogin();

		MutiLanSupport* mls = MutiLanSupport::GetInstance();
		if (error == nim::kNIMResUidPassError)
		{
 			usericon_->SetEnabled(false);
 			passwordicon_->SetEnabled(false);
			ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_PASSWORD_ERROR"));
		}
		else if (error == nim::kNIMResConnectionError)
		{
			ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_NETWORK_ERROR"));
		}
		else if (error == nim::kNIMResExist)
		{
			ShowLoginTip(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_LOCATION_CHANGED"));
		}
		else
		{
			std::wstring tip = nbase::StringPrintf(mls->GetStringViaID(L"STRID_LOGIN_FORM_TIP_ERROR_CODE").c_str(), error);
			ShowLoginTip(tip);
		}
	}
}

void LoginForm::OnCancelLogin()
{
	usericon_->SetEnabled(true);
	passwordicon_->SetEnabled(true);

	user_name_edit_->SetEnabled(true);
	password_edit_->SetEnabled(true);

	login_ing_tip_->SetVisible(false);
	login_error_tip_->SetVisible(false);

	btn_login_->SetVisible(true);
	btn_cancel_->SetVisible(false);
	btn_cancel_->SetEnabled(true);
}

void LoginForm::ShowLoginTip(std::wstring tip_text)
{
	auto task = ToWeakCallback([this, tip_text](){
		login_ing_tip_->SetVisible(false);

		login_error_tip_->SetText(tip_text);
		login_error_tip_->SetVisible(true);
	});
	Post2UI(task);
}

void LoginForm::InitLoginData()
{
	PublicDB* manager = PublicDB::GetInstance();
	UTF8String user_name = manager->GetLoginData()->user_name_;
	UTF8String user_password = manager->GetLoginData()->user_password_;
	bool remember_user = manager->GetLoginData()->remember_user_ == 1;
	bool remember_psw = manager->GetLoginData()->remember_psw_ == 1;

	if (!user_name.empty() && !user_password.empty())
	{
		if (remember_user)
		{
			user_name_edit_->SetUTF8Text(user_name);
		}

		if (remember_psw)
		{
			password_edit_->SetUTF8Text(user_password);
		}

		remember_pwd_ckb_->Selected(remember_psw);
		remember_user_ckb_->Selected(remember_user);
	}
	else
	{
		remember_pwd_ckb_->Selected(false);
		remember_user_ckb_->Selected(false);
	}
}

void LoginForm::OnLoginOK()
{
	UTF8String user_name = user_name_edit_->GetUTF8Text();
	UTF8String pass = password_edit_->GetUTF8Text();

	PublicDB::GetInstance()->GetLoginData()->status_ = kLoginDataStatusValid;
	PublicDB::GetInstance()->GetLoginData()->user_name_ = user_name;
	PublicDB::GetInstance()->GetLoginData()->user_password_ = pass;
	PublicDB::GetInstance()->GetLoginData()->remember_psw_ = remember_pwd_ckb_->IsSelected() ? 1 : 0;
	PublicDB::GetInstance()->GetLoginData()->remember_user_ = remember_user_ckb_->IsSelected() ? 1 : 0;
	PublicDB::GetInstance()->SaveLoginData();
}

void LoginForm::CheckAutoLogin()
{
	bool pwd_save = remember_pwd_ckb_->IsSelected();
	bool autorun_flag = QCommand::Get(L"autorun") == L"1";
	QLOG_APP(L"CheckAutoLogin: pwd:{0} autorun:{1}") << pwd_save << autorun_flag;
	if (pwd_save && autorun_flag)
	{
		OnLogin();
	}

}
bool LoginForm::OnSwitchToLoginPage()
{
	MutiLanSupport* multilan = MutiLanSupport::GetInstance();
	SetTaskbarTitle(multilan->GetStringViaID(L"STRID_LOGIN_FORM_LOGIN"));
	FindControl(L"nick_name_panel")->SetVisible(false);
	FindControl(L"enter_login")->SetVisible(false);
	FindControl(L"enter_panel")->SetBkImage(L"user_password.png");
	FindControl(L"login_cache_conf")->SetVisible(true);
	SetAnonymousChatroomVisible(!login_function_);
	btn_register_->SetVisible(false);
	btn_login_->SetVisible();
	user_name_edit_->SetText(L"");
	user_name_edit_->SetPromptText(multilan->GetStringViaID(L"STRID_LOGIN_FORM_ACCOUNT"));
	password_edit_->SetText(L"");
	password_edit_->SetPromptText(multilan->GetStringViaID(L"STRID_LOGIN_FORM_PASSWORD"));
	FindControl(L"register_account")->SetVisible(true);
	return true;
}