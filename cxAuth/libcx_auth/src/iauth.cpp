#include "iauth.h"
#include "hlp_auth_strings.h"

IAuth::IAuth()
{
}

IAuth::~IAuth()
{
}

bool IAuth::initAccounts()
{
    // create the default account.
    domainAdd();

    sPasswordData passData;
    passData.forceExpiration = true;
    passData.hash = createRandomString(16);

    return accountAdd("admin",  // UserName
                      passData, // Password Info
                      "",  // Domain Name
                      "", // Email
                      "Autogenerated Superuser Account", // Account Description
                      "",  // Extra Data
                      0, // Expiration (don't expire)
                      true, // enabled
                      true, // confirmed
                      true // superuser
                      );
}

AuthReason IAuth::authenticate(const std::string &accountName, const std::string &domainName, const std::string &password, uint32_t passIndex, AuthMode authMode, const std::string &cramSalt)
{
    AuthReason ret;
    bool found=false;
    Lock_Mutex_RD lock(mutex);
    sPasswordData pData = retrievePassword(accountName,domainName,passIndex, &found);
    if (found)
    {
        if (!isAccountConfirmed(accountName, domainName)) ret = AUTH_REASON_UNCONFIRMED_ACCOUNT;
        else if (isAccountDisabled(accountName, domainName)) ret = AUTH_REASON_DISABLED_ACCOUNT;
        else if (isAccountExpired(accountName, domainName)) ret = AUTH_REASON_EXPIRED_ACCOUNT;
        else ret = validatePassword(pData, password,  cramSalt, authMode);
    }
    else
        ret = AUTH_REASON_BAD_ACCOUNT;
    return ret;
}

std::string IAuth::genRandomConfirmationToken()
{
    return createRandomString(64);
}


sPasswordBasicData IAuth::accountPasswordBasicData(const std::string &accountName, bool *found, uint32_t passIndex, const std::string &domainName)
{
    // protective-limited method.
    sPasswordData pd = retrievePassword(accountName,domainName, passIndex, found);
    sPasswordBasicData pdb;

    if (!*found) pdb.passwordMode = PASS_MODE_NOTFOUND;
    else
    {
        pdb = pd.getBasicData();
    }

    return pdb;
}

bool IAuth::isAccountExpired(const std::string &accountName, const std::string &domainName)
{
    time_t tAccountExpirationDate = accountExpirationDate(accountName, domainName);
    if (!tAccountExpirationDate) return false;
    return tAccountExpirationDate<time(nullptr);
}

std::set<std::string> IAuth::accountUsableAttribs(const std::string &accountName, const std::string & domainName)
{
    std::set<std::string> x;
    Lock_Mutex_RD lock(mutex);
    // Take attribs from the account
    for (const std::string & attrib : accountDirectAttribs(accountName,domainName,false))
        x.insert(attrib);

    // Take the attribs from the belonging groups
    for (const std::string & groupName : accountGroups(accountName,domainName,false))
    {
        for (const std::string & attrib : groupAttribs(groupName,domainName,false))
            x.insert(attrib);
    }
    return x;
}

bool IAuth::accountValidateAttribute(const std::string &accountName, const std::string &attribName, std::string domainName)
{
    if (domainName == "") domainName = "default";
    Lock_Mutex_RD lock(mutex);
    if (accountValidateDirectAttribute(accountName,attribName,domainName))
    {
        return true;
    }
    for (const std::string & groupName : accountGroups(accountName, domainName,false))
    {
        if (groupValidateAttribute(groupName, domainName, attribName,false))
        {
            return true;
        }
    }
    return false;
}
