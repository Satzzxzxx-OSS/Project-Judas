#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "TextLayout.h"
class ResourceManager;class AssetDatabase;
struct LocaleEntry {std::string catalog,fallback;std::vector<std::string> fonts;};
struct ProjectLocalization {
 std::string defaultLocale="en";std::map<std::string,LocaleEntry> locales;std::vector<std::string> fonts;
 bool Validate(std::string&)const;std::string Encode()const;static bool Parse(const std::string&,ProjectLocalization&,std::string&);
};
struct Catalog {std::string locale;std::map<std::string,std::string> messages;};
bool ParseCatalog(const std::string&,Catalog&,std::string&);bool LoadCatalog(const std::string&,Catalog&,std::string&);
bool ValidateLocalizationAssets(const ProjectLocalization&,const AssetDatabase&,std::string&);
using MessageArguments=std::map<std::string,std::variant<double,std::string>>;
struct NumberOptions {int minimumFraction=0,maximumFraction=3;bool grouping=true;};
// One instance per played project session, retained through scene replacement.
// All methods are owner-thread; catalogs/bytes prepared by ordinary resource workers.
class LocalizationSession {
public:
 explicit LocalizationSession(ProjectLocalization config={});~LocalizationSession();
 void Bind(ResourceManager*);void Refresh();void Reload();bool SetLocale(const std::string&,std::string&);
 const std::string& Locale()const;const ProjectLocalization& Configuration()const;
 uint64_t Revision()const;bool RTL()const;
 std::string Format(const std::string&,const MessageArguments&,std::string&);
 std::string Number(double,const NumberOptions&,std::string&);
 std::vector<std::shared_ptr<const TextFont>> Fonts();
private:struct Impl;std::unique_ptr<Impl> m;
};
