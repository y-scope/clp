type Settings = {
    MongoDbSearchResultsMetadataCollectionName: string;
};

/**
 * Loads application settings from a `settings.json` file.
 *
 * This function fetches a JSON file named `settings.json` from the server
 * and parses its contents into a `Settings` object.
 *
 * @return
 * @throws {Error} If the fetch or JSON parsing fails, an error is thrown with the original cause.
 */
const loadSettings = async (): Promise<Settings> => {
    try {
        const response = await fetch("settings.json");
        return (await response.json()) as Settings;
    } catch (e: unknown) {
        throw new Error("Failed to fetch settings.", {cause: e});
    }
};

const settings: Settings = await loadSettings();


export {settings};
