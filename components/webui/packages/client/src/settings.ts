import type {WebuiPublicSettings} from "@webui/common/schemas/settings";
import axios from "axios";


/**
 * Loads application settings from a `settings.json` file.
 *
 * This function fetches a JSON file named `settings.json` from the server and parses its contents
 * into a `WebuiPublicSettings` object.
 *
 * @return
 * @throws {Error} If the fetch or JSON parsing fails, an error is thrown with the original cause.
 */
const loadSettings = async (): Promise<WebuiPublicSettings> => {
    try {
        const response = await axios.get<WebuiPublicSettings>("settings.json");
        return response.data;
    } catch (e: unknown) {
        throw new Error("Failed to fetch settings.", {cause: e});
    }
};

const settings: WebuiPublicSettings = await loadSettings();


export {settings};
