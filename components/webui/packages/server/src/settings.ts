import {
    existsSync,
    readFileSync,
} from "node:fs";
import path from "node:path";
import {fileURLToPath} from "node:url";

import {Value} from "@sinclair/typebox/value";
import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {WebuiSettingsSchema} from "@webui/common/schemas/settings";


const moduleDir = path.dirname(fileURLToPath(import.meta.url));
const settingsPathCandidates = [
    path.resolve(moduleDir, "../../../../../etc/webui/settings.json"),
    path.resolve(moduleDir, "../../../settings.json"),
];

/**
 * Resolves the settings path for installed-package and source layouts.
 *
 * @return The resolved settings file path.
 * @throws {Error} If no settings file can be located.
 */
const resolveSettingsPath = (): string => {
    for (const candidate of settingsPathCandidates) {
        if (existsSync(candidate)) {
            return candidate;
        }
    }

    throw new Error(
        `Unable to locate the WebUI settings file. Ensure one of these exists: ${
            settingsPathCandidates.join(", ")}`
    );
};

const settingsPath = resolveSettingsPath();

const rawSettings: unknown = JSON.parse(readFileSync(settingsPath, "utf8"));

const settings = Value.Parse(WebuiSettingsSchema, rawSettings);

const s3ScannerEnabled = CLP_STORAGE_ENGINES.CLP_S === settings.public.ClpStorageEngine &&
    STORAGE_TYPE.S3 === settings.public.LogsInputType && null !== settings.server.LogIngestorUrl;
const publicSettings = {...settings.public, S3ScannerEnabled: s3ScannerEnabled};
const serverSettings = settings.server;

export {
    publicSettings,
    serverSettings,
    settings,
    settingsPath,
};
