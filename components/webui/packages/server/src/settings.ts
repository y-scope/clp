import {
    existsSync,
    readFileSync,
} from "node:fs";
import path from "node:path";
import {fileURLToPath} from "node:url";

import {Value} from "@sinclair/typebox/value";
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

const publicSettings = settings.public;
const serverSettings = settings.server;

export {
    publicSettings,
    serverSettings,
    settings,
    settingsPath,
};
