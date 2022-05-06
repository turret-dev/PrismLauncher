#include "FlameModIndex.h"

#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/flame/FlameAPI.h"
#include "net/NetJob.h"

static ModPlatform::ProviderCapabilities ProviderCaps;

void FlameMod::loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.addonId = Json::requireInteger(obj, "id");
    pack.provider = ModPlatform::Provider::FLAME;
    pack.name = Json::requireString(obj, "name");
    pack.websiteUrl = Json::ensureString(Json::ensureObject(obj, "links"), "websiteUrl", "");
    pack.description = Json::ensureString(obj, "summary", "");

    QJsonObject logo = Json::requireObject(obj, "logo");
    pack.logoName = Json::requireString(logo, "title");
    pack.logoUrl = Json::requireString(logo, "thumbnailUrl");

    auto authors = Json::requireArray(obj, "authors");
    for (auto authorIter : authors) {
        auto author = Json::requireObject(authorIter);
        ModPlatform::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }
}

void FlameMod::loadIndexedPackVersions(ModPlatform::IndexedPack& pack,
                                       QJsonArray& arr,
                                       const shared_qobject_ptr<QNetworkAccessManager>& network,
                                       BaseInstance* inst)
{
    QVector<ModPlatform::IndexedVersion> unsortedVersions;
    auto profile = (dynamic_cast<MinecraftInstance*>(inst))->getPackProfile();
    QString mcVersion = profile->getComponentVersion("net.minecraft");

    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        auto versionArray = Json::requireArray(obj, "gameVersions");
        if (versionArray.isEmpty()) {
            continue;
        }

        ModPlatform::IndexedVersion file;
        for (auto mcVer : versionArray) {
            auto str = mcVer.toString();

            if (str.contains('.'))
                file.mcVersion.append(str);
        }

        file.addonId = pack.addonId;
        file.fileId = Json::requireInteger(obj, "id");
        file.date = Json::requireString(obj, "fileDate");
        file.version = Json::requireString(obj, "displayName");
        file.downloadUrl = Json::requireString(obj, "downloadUrl");
        file.fileName = Json::requireString(obj, "fileName");

        auto hash_list = Json::ensureArray(obj, "hashes");
        if(!hash_list.isEmpty()){
            auto hash_types = ProviderCaps.hashType(ModPlatform::Provider::FLAME);
            for(auto& hash_type : hash_types) {
                if(hash_list.contains(hash_type)) {
                    file.hash = Json::requireString(hash_list, "value");
                    file.hash_type = hash_type;
                    break;
                }
            }
        }

        unsortedVersions.append(file);
    }

    auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
        // dates are in RFC 3339 format
        return a.date > b.date;
    };
    std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);
    pack.versions = unsortedVersions;
    pack.versionsLoaded = true;
}
