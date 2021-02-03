/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-02-02
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Steam
{
	enum class EResult
	{
		k_EResultNone = 0,							// no result
		k_EResultOK = 1,							// success
		k_EResultFail = 2,							// generic failure
		k_EResultNoConnection = 3,					// no/failed network connection
		k_EResultNoConnectionRetry = 4,				// OBSOLETE - removed
		k_EResultInvalidPassword = 5,				// password/ticket is invalid
		k_EResultLoggedInElsewhere = 6,				// same user logged in elsewhere
		k_EResultInvalidProtocolVer = 7,			// protocol version is incorrect
		k_EResultInvalidParam = 8,					// a parameter is incorrect
		k_EResultFileNotFound = 9,					// file was not found
		k_EResultBusy = 10,							// called method busy - action not taken
		k_EResultInvalidState = 11,					// called object was in an invalid state
		k_EResultInvalidName = 12,					// name is invalid
		k_EResultInvalidEmail = 13,					// email is invalid
		k_EResultDuplicateName = 14,				// name is not unique
		k_EResultAccessDenied = 15,					// access is denied
		k_EResultTimeout = 16,						// operation timed out
		k_EResultBanned = 17,						// VAC2 banned
		k_EResultAccountNotFound = 18,				// account not found
		k_EResultInvalidSteamID = 19,				// steamID is invalid
		k_EResultServiceUnavailable = 20,			// The requested service is currently unavailable
		k_EResultNotLoggedOn = 21,					// The user is not logged on
		k_EResultPending = 22,						// Request is pending (may be in process, or waiting on third party)
		k_EResultEncryptionFailure = 23,			// Encryption or Decryption failed
		k_EResultInsufficientPrivilege = 24,		// Insufficient privilege
		k_EResultLimitExceeded = 25,				// Too much of a good thing
		k_EResultRevoked = 26,						// Access has been revoked (used for revoked guest passes)
		k_EResultExpired = 27,						// License/Guest pass the user is trying to access is expired
		k_EResultAlreadyRedeemed = 28,				// Guest pass has already been redeemed by account, cannot be acked again
		k_EResultDuplicateRequest = 29,				// The request is a duplicate and the action has already occurred in the past, ignored this time
		k_EResultAlreadyOwned = 30,					// All the games in this guest pass redemption request are already owned by the user
		k_EResultIPNotFound = 31,					// IP address not found
		k_EResultPersistFailed = 32,				// failed to write change to the data store
		k_EResultLockingFailed = 33,				// failed to acquire access lock for this operation
		k_EResultLogonSessionReplaced = 34,
		k_EResultConnectFailed = 35,
		k_EResultHandshakeFailed = 36,
		k_EResultIOFailure = 37,
		k_EResultRemoteDisconnect = 38,
		k_EResultShoppingCartNotFound = 39,			// failed to find the shopping cart requested
		k_EResultBlocked = 40,						// a user didn't allow it
		k_EResultIgnored = 41,						// target is ignoring sender
		k_EResultNoMatch = 42,						// nothing matching the request found
		k_EResultAccountDisabled = 43,
		k_EResultServiceReadOnly = 44,				// this service is not accepting content changes right now
		k_EResultAccountNotFeatured = 45,			// account doesn't have value, so this feature isn't available
		k_EResultAdministratorOK = 46,				// allowed to take this action, but only because requester is admin
		k_EResultContentVersion = 47,				// A Version mismatch in content transmitted within the Steam protocol.
		k_EResultTryAnotherCM = 48,					// The current CM can't service the user making a request, user should try another.
		k_EResultPasswordRequiredToKickSession = 49,// You are already logged in elsewhere, this cached credential login has failed.
		k_EResultAlreadyLoggedInElsewhere = 50,		// You are already logged in elsewhere, you must wait
		k_EResultSuspended = 51,					// Long running operation (content download) suspended/paused
		k_EResultCancelled = 52,					// Operation canceled (typically by user: content download)
		k_EResultDataCorruption = 53,				// Operation canceled because data is ill formed or unrecoverable
		k_EResultDiskFull = 54,						// Operation canceled - not enough disk space.
		k_EResultRemoteCallFailed = 55,				// an remote call or IPC call failed
		k_EResultPasswordUnset = 56,				// Password could not be verified as it's unset server side
		k_EResultExternalAccountUnlinked = 57,		// External account (PSN, Facebook...) is not linked to a Steam account
		k_EResultPSNTicketInvalid = 58,				// PSN ticket was invalid
		k_EResultExternalAccountAlreadyLinked = 59,	// External account (PSN, Facebook...) is already linked to some other account, must explicitly request to replace/delete the link first
		k_EResultRemoteFileConflict = 60,			// The sync cannot resume due to a conflict between the local and remote files
		k_EResultIllegalPassword = 61,				// The requested new password is not legal
		k_EResultSameAsPreviousValue = 62,			// new value is the same as the old one ( secret question and answer )
		k_EResultAccountLogonDenied = 63,			// account login denied due to 2nd factor authentication failure
		k_EResultCannotUseOldPassword = 64,			// The requested new password is not legal
		k_EResultInvalidLoginAuthCode = 65,			// account login denied due to auth code invalid
		k_EResultAccountLogonDeniedNoMail = 66,		// account login denied due to 2nd factor auth failure - and no mail has been sent
		k_EResultHardwareNotCapableOfIPT = 67,		//
		k_EResultIPTInitError = 68,					//
		k_EResultParentalControlRestricted = 69,	// operation failed due to parental control restrictions for current user
		k_EResultFacebookQueryError = 70,			// Facebook query returned an error
		k_EResultExpiredLoginAuthCode = 71,			// account login denied due to auth code expired
		k_EResultIPLoginRestrictionFailed = 72,
		k_EResultAccountLockedDown = 73,
		k_EResultAccountLogonDeniedVerifiedEmailRequired = 74,
		k_EResultNoMatchingURL = 75,
		k_EResultBadResponse = 76,					// parse failure, missing field, etc.
		k_EResultRequirePasswordReEntry = 77,		// The user cannot complete the action until they re-enter their password
		k_EResultValueOutOfRange = 78,				// the value entered is outside the acceptable range
		k_EResultUnexpectedError = 79,				// something happened that we didn't expect to ever happen
		k_EResultDisabled = 80,						// The requested service has been configured to be unavailable
		k_EResultInvalidCEGSubmission = 81,			// The set of files submitted to the CEG server are not valid !
		k_EResultRestrictedDevice = 82,				// The device being used is not allowed to perform this action
		k_EResultRegionLocked = 83,					// The action could not be complete because it is region restricted
		k_EResultRateLimitExceeded = 84,			// Temporary rate limit exceeded, try again later, different from k_EResultLimitExceeded which may be permanent
		k_EResultAccountLoginDeniedNeedTwoFactor = 85,	// Need two-factor code to login
		k_EResultItemDeleted = 86,					// The thing we're trying to access has been deleted
		k_EResultAccountLoginDeniedThrottle = 87,	// login attempt failed, try to throttle response to possible attacker
		k_EResultTwoFactorCodeMismatch = 88,		// two factor code mismatch
		k_EResultTwoFactorActivationCodeMismatch = 89,	// activation code for two-factor didn't match
		k_EResultAccountAssociatedToMultiplePartners = 90,	// account has been associated with multiple partners
		k_EResultNotModified = 91,					// data not modified
		k_EResultNoMobileDevice = 92,				// the account does not have a mobile device associated with it
		k_EResultTimeNotSynced = 93,				// the time presented is out of range or tolerance
		k_EResultSmsCodeFailed = 94,				// SMS code failure (no match, none pending, etc.)
		k_EResultAccountLimitExceeded = 95,			// Too many accounts access this resource
		k_EResultAccountActivityLimitExceeded = 96,	// Too many changes to this account
		k_EResultPhoneActivityLimitExceeded = 97,	// Too many changes to this phone
		k_EResultRefundToWallet = 98,				// Cannot refund to payment method, must use wallet
		k_EResultEmailSendFailure = 99,				// Cannot send an email
		k_EResultNotSettled = 100,					// Can't perform operation till payment has settled
		k_EResultNeedCaptcha = 101,					// Needs to provide a valid captcha
		k_EResultGSLTDenied = 102,					// a game server login token owned by this token's owner has been banned
		k_EResultGSOwnerDenied = 103,				// game server owner is denied for other reason (account lock, community ban, vac ban, missing phone)
		k_EResultInvalidItemType = 104,				// the type of thing we were requested to act on is invalid
		k_EResultIPBanned = 105,					// the ip address has been banned from taking this action
		k_EResultGSLTExpired = 106,					// this token has expired from disuse; can be reset for use
		k_EResultInsufficientFunds = 107,			// user doesn't have enough wallet funds to complete the action
		k_EResultTooManyPending = 108,				// There are too many of this thing pending already
		k_EResultNoSiteLicensesFound = 109,			// No site licenses found
		k_EResultWGNetworkSendExceeded = 110,		// the WG couldn't send a response because we exceeded max network send size
		k_EResultAccountNotFriends = 111,			// the user is not mutually friends
		k_EResultLimitedUserAccount = 112,			// the user is limited
		k_EResultCantRemoveItem = 113,				// item can't be removed
		k_EResultAccountDeleted = 114,				// account has been deleted
		k_EResultExistingUserCancelledLicense = 115,	// A license for this already exists, but cancelled
		k_EResultCommunityCooldown = 116,			// access is denied because of a community cooldown (probably from support profile data resets)
		k_EResultNoLauncherSpecified = 117,			// No launcher was specified, but a launcher was needed to choose correct realm for operation.
		k_EResultMustAgreeToSSA = 118,				// User must agree to china SSA or global SSA before login
		k_EResultLauncherMigrated = 119,			// The specified launcher type is no longer supported; the user should be directed elsewhere
	};

	constexpr std::string asString(EResult Code)
	{
		switch (Code)
		{
			case EResult::k_EResultNone: return "k_EResultNone";
			case EResult::k_EResultOK: return "k_EResultOK";
			case EResult::k_EResultFail: return "k_EResultFail";
			case EResult::k_EResultNoConnection: return "k_EResultNoConnection";
			case EResult::k_EResultNoConnectionRetry: return "k_EResultNoConnectionRetry";
			case EResult::k_EResultInvalidPassword: return "k_EResultInvalidPassword";
			case EResult::k_EResultLoggedInElsewhere: return "k_EResultLoggedInElsewhere";
			case EResult::k_EResultInvalidProtocolVer: return "k_EResultInvalidProtocolVer";
			case EResult::k_EResultInvalidParam: return "k_EResultInvalidParam";
			case EResult::k_EResultFileNotFound: return "k_EResultFileNotFound";
			case EResult::k_EResultBusy: return "k_EResultBusy";
			case EResult::k_EResultInvalidState: return "k_EResultInvalidState";
			case EResult::k_EResultInvalidName: return "k_EResultInvalidName";
			case EResult::k_EResultInvalidEmail: return "k_EResultInvalidEmail";
			case EResult::k_EResultDuplicateName: return "k_EResultDuplicateName";
			case EResult::k_EResultAccessDenied: return "k_EResultAccessDenied";
			case EResult::k_EResultTimeout: return "k_EResultTimeout";
			case EResult::k_EResultBanned: return "k_EResultBanned";
			case EResult::k_EResultAccountNotFound: return "k_EResultAccountNotFound";
			case EResult::k_EResultInvalidSteamID: return "k_EResultInvalidSteamID";
			case EResult::k_EResultServiceUnavailable: return "k_EResultServiceUnavailable";
			case EResult::k_EResultNotLoggedOn: return "k_EResultNotLoggedOn";
			case EResult::k_EResultPending: return "k_EResultPending";
			case EResult::k_EResultEncryptionFailure: return "k_EResultEncryptionFailure";
			case EResult::k_EResultInsufficientPrivilege: return "k_EResultInsufficientPrivilege";
			case EResult::k_EResultLimitExceeded: return "k_EResultLimitExceeded";
			case EResult::k_EResultRevoked: return "k_EResultRevoked";
			case EResult::k_EResultExpired: return "k_EResultExpired";
			case EResult::k_EResultAlreadyRedeemed: return "k_EResultAlreadyRedeemed";
			case EResult::k_EResultDuplicateRequest: return "k_EResultDuplicateRequest";
			case EResult::k_EResultAlreadyOwned: return "k_EResultAlreadyOwned";
			case EResult::k_EResultIPNotFound: return "k_EResultIPNotFound";
			case EResult::k_EResultPersistFailed: return "k_EResultPersistFailed";
			case EResult::k_EResultLockingFailed: return "k_EResultLockingFailed";
			case EResult::k_EResultLogonSessionReplaced: return "k_EResultLogonSessionReplaced";
			case EResult::k_EResultConnectFailed: return "k_EResultConnectFailed";
			case EResult::k_EResultHandshakeFailed: return "k_EResultHandshakeFailed";
			case EResult::k_EResultIOFailure: return "k_EResultIOFailure";
			case EResult::k_EResultRemoteDisconnect: return "k_EResultRemoteDisconnect";
			case EResult::k_EResultShoppingCartNotFound: return "k_EResultShoppingCartNotFound";
			case EResult::k_EResultBlocked: return "k_EResultBlocked";
			case EResult::k_EResultIgnored: return "k_EResultIgnored";
			case EResult::k_EResultNoMatch: return "k_EResultNoMatch";
			case EResult::k_EResultAccountDisabled: return "k_EResultAccountDisabled";
			case EResult::k_EResultServiceReadOnly: return "k_EResultServiceReadOnly";
			case EResult::k_EResultAccountNotFeatured: return "k_EResultAccountNotFeatured";
			case EResult::k_EResultAdministratorOK: return "k_EResultAdministratorOK";
			case EResult::k_EResultContentVersion: return "k_EResultContentVersion";
			case EResult::k_EResultTryAnotherCM: return "k_EResultTryAnotherCM";
			case EResult::k_EResultPasswordRequiredToKickSession: return "k_EResultPasswordRequiredToKickSession";
			case EResult::k_EResultAlreadyLoggedInElsewhere: return "k_EResultAlreadyLoggedInElsewhere";
			case EResult::k_EResultSuspended: return "k_EResultSuspended";
			case EResult::k_EResultCancelled: return "k_EResultCancelled";
			case EResult::k_EResultDataCorruption: return "k_EResultDataCorruption";
			case EResult::k_EResultDiskFull: return "k_EResultDiskFull";
			case EResult::k_EResultRemoteCallFailed: return "k_EResultRemoteCallFailed";
			case EResult::k_EResultPasswordUnset: return "k_EResultPasswordUnset";
			case EResult::k_EResultExternalAccountUnlinked: return "k_EResultExternalAccountUnlinked";
			case EResult::k_EResultPSNTicketInvalid: return "k_EResultPSNTicketInvalid";
			case EResult::k_EResultExternalAccountAlreadyLinked: return "k_EResultExternalAccountAlreadyLinked";
			case EResult::k_EResultRemoteFileConflict: return "k_EResultRemoteFileConflict";
			case EResult::k_EResultIllegalPassword: return "k_EResultIllegalPassword";
			case EResult::k_EResultSameAsPreviousValue: return "k_EResultSameAsPreviousValue";
			case EResult::k_EResultAccountLogonDenied: return "k_EResultAccountLogonDenied";
			case EResult::k_EResultCannotUseOldPassword: return "k_EResultCannotUseOldPassword";
			case EResult::k_EResultInvalidLoginAuthCode: return "k_EResultInvalidLoginAuthCode";
			case EResult::k_EResultAccountLogonDeniedNoMail: return "k_EResultAccountLogonDeniedNoMail";
			case EResult::k_EResultHardwareNotCapableOfIPT: return "k_EResultHardwareNotCapableOfIPT";
			case EResult::k_EResultIPTInitError: return "k_EResultIPTInitError";
			case EResult::k_EResultParentalControlRestricted: return "k_EResultParentalControlRestricted";
			case EResult::k_EResultFacebookQueryError: return "k_EResultFacebookQueryError";
			case EResult::k_EResultExpiredLoginAuthCode: return "k_EResultExpiredLoginAuthCode";
			case EResult::k_EResultIPLoginRestrictionFailed: return "k_EResultIPLoginRestrictionFailed";
			case EResult::k_EResultAccountLockedDown: return "k_EResultAccountLockedDown";
			case EResult::k_EResultAccountLogonDeniedVerifiedEmailRequired: return "k_EResultAccountLogonDeniedVerifiedEmailRequired";
			case EResult::k_EResultNoMatchingURL: return "k_EResultNoMatchingURL";
			case EResult::k_EResultBadResponse: return "k_EResultBadResponse";
			case EResult::k_EResultRequirePasswordReEntry: return "k_EResultRequirePasswordReEntry";
			case EResult::k_EResultValueOutOfRange: return "k_EResultValueOutOfRange";
			case EResult::k_EResultUnexpectedError: return "k_EResultUnexpectedError";
			case EResult::k_EResultDisabled: return "k_EResultDisabled";
			case EResult::k_EResultInvalidCEGSubmission: return "k_EResultInvalidCEGSubmission";
			case EResult::k_EResultRestrictedDevice: return "k_EResultRestrictedDevice";
			case EResult::k_EResultRegionLocked: return "k_EResultRegionLocked";
			case EResult::k_EResultRateLimitExceeded: return "k_EResultRateLimitExceeded";
			case EResult::k_EResultAccountLoginDeniedNeedTwoFactor: return "k_EResultAccountLoginDeniedNeedTwoFactor";
			case EResult::k_EResultItemDeleted: return "k_EResultItemDeleted";
			case EResult::k_EResultAccountLoginDeniedThrottle: return "k_EResultAccountLoginDeniedThrottle";
			case EResult::k_EResultTwoFactorCodeMismatch: return "k_EResultTwoFactorCodeMismatch";
			case EResult::k_EResultTwoFactorActivationCodeMismatch: return "k_EResultTwoFactorActivationCodeMismatch";
			case EResult::k_EResultAccountAssociatedToMultiplePartners: return "k_EResultAccountAssociatedToMultiplePartners";
			case EResult::k_EResultNotModified: return "k_EResultNotModified";
			case EResult::k_EResultNoMobileDevice: return "k_EResultNoMobileDevice";
			case EResult::k_EResultTimeNotSynced: return "k_EResultTimeNotSynced";
			case EResult::k_EResultSmsCodeFailed: return "k_EResultSmsCodeFailed";
			case EResult::k_EResultAccountLimitExceeded: return "k_EResultAccountLimitExceeded";
			case EResult::k_EResultAccountActivityLimitExceeded: return "k_EResultAccountActivityLimitExceeded";
			case EResult::k_EResultPhoneActivityLimitExceeded: return "k_EResultPhoneActivityLimitExceeded";
			case EResult::k_EResultRefundToWallet: return "k_EResultRefundToWallet";
			case EResult::k_EResultEmailSendFailure: return "k_EResultEmailSendFailure";
			case EResult::k_EResultNotSettled: return "k_EResultNotSettled";
			case EResult::k_EResultNeedCaptcha: return "k_EResultNeedCaptcha";
			case EResult::k_EResultGSLTDenied: return "k_EResultGSLTDenied";
			case EResult::k_EResultGSOwnerDenied: return "k_EResultGSOwnerDenied";
			case EResult::k_EResultInvalidItemType: return "k_EResultInvalidItemType";
			case EResult::k_EResultIPBanned: return "k_EResultIPBanned";
			case EResult::k_EResultGSLTExpired: return "k_EResultGSLTExpired";
			case EResult::k_EResultInsufficientFunds: return "k_EResultInsufficientFunds";
			case EResult::k_EResultTooManyPending: return "k_EResultTooManyPending";
			case EResult::k_EResultNoSiteLicensesFound: return "k_EResultNoSiteLicensesFound";
			case EResult::k_EResultWGNetworkSendExceeded: return "k_EResultWGNetworkSendExceeded";
			case EResult::k_EResultAccountNotFriends: return "k_EResultAccountNotFriends";
			case EResult::k_EResultLimitedUserAccount: return "k_EResultLimitedUserAccount";
			case EResult::k_EResultCantRemoveItem: return "k_EResultCantRemoveItem";
			case EResult::k_EResultAccountDeleted: return "k_EResultAccountDeleted";
			case EResult::k_EResultExistingUserCancelledLicense: return "k_EResultExistingUserCancelledLicense";
			case EResult::k_EResultCommunityCooldown: return "k_EResultCommunityCooldown";
			case EResult::k_EResultNoLauncherSpecified: return "k_EResultNoLauncherSpecified";
			case EResult::k_EResultMustAgreeToSSA: return "k_EResultMustAgreeToSSA";
			case EResult::k_EResultLauncherMigrated: return "k_EResultLauncherMigrated";
			default: return "unknown";
		}
	}
}