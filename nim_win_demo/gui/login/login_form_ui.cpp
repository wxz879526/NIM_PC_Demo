#include "resource.h"
#include "login_form.h"
#include "util/user.h"
#include "shared/tool.h"
#include "gui/main/main_form.h"
#include "gui/proxy/proxy_form.h"
#include "module/db/public_db.h"
#include "gui/chatroom_frontpage.h"
#include "module\config\config_helper.h"
using namespace ui;

const LPCTSTR LoginForm::kClassName	= L"LoginForm";

LoginForm::LoginForm():
login_function_(false)
{
	PublicDB::GetInstance()->ReadLoginData();
}

LoginForm::~LoginForm()
{

}

std::wstring LoginForm::GetSkinFolder()
{
	return L"login";
}

std::wstring LoginForm::GetSkinFile()
{
	return L"login.xml";
}

std::wstring LoginForm::GetWindowClassName() const 
{
	return kClassName;
}

std::wstring LoginForm::GetWindowId() const
{
	return kClassName;
}

UINT LoginForm::GetClassStyle() const 
{
	return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
}

void LoginForm::InitWindow()
{
	SetIcon(IDI_ICON);
	m_pRoot->AttachBubbledEvent(ui::kEventAll, nbase::Bind(&LoginForm::Notify, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&LoginForm::OnClicked, this, std::placeholders::_1));

	usericon_ = FindControl(L"usericon");
	passwordicon_ = FindControl(L"passwordicon");

	user_name_edit_ = (RichEdit*) FindControl( L"username" );
	nick_name_edit_ = (RichEdit*)FindControl(L"nickname");
	password_edit_ = (RichEdit*) FindControl( L"password" );
	user_name_edit_->SetSelAllOnFocus(true);
	password_edit_->SetSelAllOnFocus(true);

	login_ing_tip_ = FindControl( L"login_ing_tip" );
	login_error_tip_ = (Label*) FindControl( L"login_error_tip" );
	register_ok_toast_ = (Label*)FindControl(L"register_ok_toast");

	btn_login_ = (Button*) FindControl( L"btn_login" );
	btn_register_ = (Button*)FindControl(L"btn_register");
	btn_cancel_ = (Button*) FindControl( L"btn_cancel" );
	remember_pwd_ckb_ = (CheckBox *)FindControl(L"chkbox_remember_pwd");
	remember_user_ckb_ = (CheckBox *)FindControl(L"chkbox_remember_username");

	use_new_uistyle_ = (CheckBox *)FindControl(L"chkbox_use_new_uistyle");
	use_new_uistyle_->Selected(ConfigHelper::GetInstance()->GetUIStyle() == 1);
	use_new_uistyle_->AttachSelect([](ui::EventArgs* param){
		ConfigHelper::GetInstance()->SetUIStyle(1);
		return true;
	});
	use_new_uistyle_->AttachUnSelect([](ui::EventArgs* param){
		ConfigHelper::GetInstance()->SetUIStyle(0);
		return true;
	});
	//RichEdit��SetText����������󣬻ᴥ��TextChange�¼�
	std::wstring account = QCommand::Get(kCmdAccount);
	user_name_edit_->SetText(account);

	MutiLanSupport* multilan = MutiLanSupport::GetInstance();
	std::wstring why = QCommand::Get(kCmdExitWhy);
	if( !why.empty() )
	{
		int reason = _wtoi( why.c_str() );
		if (reason == nim::kNIMLogoutKickout)
			ShowLoginTip(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_KICKED"));
		else if (reason == nim::kNIMLogoutRelogin)
			ShowLoginTip(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_NETWAORK_DISCONNECTED"));
		else if (reason == nim::kNIMResExist)
			ShowLoginTip(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_LOCATION_CHANGED"));
		else
			ShowLoginTip(nbase::StringPrintf(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_ERROR_CODE").c_str(), reason));

		QCommand::Erase(kCmdExitWhy);
	}

	user_name_edit_->SetLimitText(32);
	nick_name_edit_->SetLimitText(64);
	password_edit_->SetLimitText(128);
	((ui::Button*)FindControl(L"register_account"))->AttachClick([this](ui::EventArgs* msg) {
		MutiLanSupport* multilan = MutiLanSupport::GetInstance();
		SetTaskbarTitle(multilan->GetStringViaID(L"STRID_LOGIN_FORM_REGISTER"));
		FindControl(L"enter_panel")->SetBkImage(L"user_password_nickname.png");
		FindControl(L"login_cache_conf")->SetVisible(false);
		SetAnonymousChatroomVisible(false);
		msg->pSender->GetWindow()->FindControl(L"nick_name_panel")->SetVisible();
		msg->pSender->GetWindow()->FindControl(L"enter_login")->SetVisible();
		btn_register_->SetVisible();
		btn_login_->SetVisible(false);
		user_name_edit_->SetText(L"");
		user_name_edit_->SetPromptText(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_ACCOUNT_RESTRICT"));
		usericon_->SetEnabled(true);
		password_edit_->SetText(L"");
		password_edit_->SetPromptText(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_PASSWORD_RESTRICT"));
		passwordicon_->SetEnabled(true);
		nick_name_edit_->SetText(L"");
		nick_name_edit_->SetPromptText(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_NICKNAME_RESTRICT"));
		msg->pSender->GetWindow()->FindControl(L"register_account")->SetVisible(false);
		return true; });

	((ui::Button*)FindControl(L"enter_login"))->AttachClick([this](ui::EventArgs* msg) {
		return OnSwitchToLoginPage();
		//MutiLanSupport* multilan = MutiLanSupport::GetInstance();
		//SetTaskbarTitle(multilan->GetStringViaID(L"STRID_LOGIN_FORM_LOGIN"));
		//FindControl(L"enter_panel")->SetBkImage(L"user_password.png");		
		//msg->pSender->GetWindow()->FindControl(L"nick_name_panel")->SetVisible(false);
		//msg->pSender->GetWindow()->FindControl(L"enter_login")->SetVisible(false);
		//FindControl(L"login_cache_conf")->SetVisible(true);
		//SetAnonymousChatroomVisible(true);
		//btn_register_->SetVisible(false);
		//btn_login_->SetVisible();
		//user_name_edit_->SetText(L"");
		//user_name_edit_->SetPromptText(multilan->GetStringViaID(L"STRID_LOGIN_FORM_ACCOUNT"));
		//password_edit_->SetText(L"");
		//password_edit_->SetPromptText(multilan->GetStringViaID(L"STRID_LOGIN_FORM_PASSWORD"));
		//msg->pSender->GetWindow()->FindControl(L"register_account")->SetVisible(true);
		return true; });

	this->RegLoginManagerCallback();

	InitLoginData();
	CheckAutoLogin();	

	std::vector<shared::tools::FileItem> allList;
	shared::tools::File::GetFile(L"E:\\code\\common\\6_wxz\\Meta Trader4 Test\\bin\\skin", allList, true);
	int x = 0;
	++x;
}

LRESULT LoginForm::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT LoginForm::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!login_function_)
	{
		nim_ui::LoginManager::GetInstance()->DoLogout(false);
		return 1;
	}

	return __super::OnClose(uMsg, wParam, lParam, bHandled);
}

bool LoginForm::Notify( ui::EventArgs* msg )
{
	std::wstring name = msg->pSender->GetName();
	if(msg->Type == kEventTextChange)
	{
		if(name == L"username")
		{
			OnCancelLogin();
			password_edit_->SetText(L"");
		}
		else if(name == L"password")
		{
			//ȥ�������ַ�
			bool has_chinise = false;
			std::wstring text = password_edit_->GetText(), text_fixed;
			for (size_t i = 0; i < text.length(); i++)
			{
				if (IsAsciiChar(text[i]))
					text_fixed.push_back(text[i]);
				else
					has_chinise = true;
			}
			password_edit_->SetText(text_fixed);

			if (has_chinise)
			{
				MutiLanSupport* multilan = MutiLanSupport::GetInstance();
				ShowLoginTip(multilan->GetStringViaID(L"STRID_LOGIN_FORM_TIP_PASSWORD_RESTRICT2"));
				passwordicon_->SetEnabled(false);
			}
			else 
			{
				login_error_tip_->SetVisible(false);
				passwordicon_->SetEnabled(true);
			}
		}
		else if (name == L"nickname")
		{
			login_error_tip_->SetVisible(false);
			FindControl(L"nick_name_icon")->SetEnabled(true);
		}
	}
	else if(msg->Type == kEventTab)
	{
		if(user_name_edit_->IsFocused())
		{
			if (nick_name_edit_->IsVisible())
			{
				nick_name_edit_->SetSelAll();
				nick_name_edit_->SetFocus();
			}
			else
			{
				password_edit_->SetSelAll();
				password_edit_->SetFocus();
			}
		}
		else if(password_edit_->IsFocused())
		{
			user_name_edit_->SetSelAll();
			user_name_edit_->SetFocus();
		}
		else if (nick_name_edit_->IsFocused())
		{
			password_edit_->SetSelAll();
			password_edit_->SetFocus();
		}
	}
	else if (msg->Type == kEventReturn)
	{
		if (name == L"username" || name == L"nickname" || name == L"password")
		{
			if (!nick_name_edit_->IsVisible())
			{
				btn_login_->SetFocus();
				OnLogin();
			}
			else
				DoRegisterAccount();
		}
	}
	return true;
}

bool LoginForm::OnClicked( ui::EventArgs* msg )
{
	std::wstring name = msg->pSender->GetName();
	if(name == L"btn_login")
	{
		OnLogin();
	}
	else if (name == L"btn_register")
	{
		DoRegisterAccount();
	}
	else if(name == L"btn_cancel")
	{
		btn_cancel_->SetEnabled(false);
		nim_ui::LoginManager::GetInstance()->CancelLogin();
	}
	else if (name == L"proxy_setting")
	{
		nim_comp::WindowsManager::SingletonShow<ProxyForm>(ProxyForm::kClassName);
	}
	else if (name == L"anonymous_chatroom")
	{
		DoInitUiKit(nim_ui::InitManager::kAnonymousChatroom);
		nim_comp::LoginManager::GetInstance()->SetAnonymityDemoMode();
		ShowWindow(false, false);
		auto form = nim_ui::WindowsManager::GetInstance()->SingletonShow<nim_chatroom::ChatroomFrontpage>(nim_chatroom::ChatroomFrontpage::kClassName);
		form->SetAnonymity(true);
	}
	return true;
}

void LoginForm::SetAnonymousChatroomVisible(bool visible)
{
	FindControl(L"anonymous_chatroom")->SetVisible(visible);
}
void LoginForm::SwitchToLoginPage()
{
	OnSwitchToLoginPage();
}
