import {Meteor} from "meteor/meteor";

import "/imports/api/search/server/methods";
import "/imports/api/search/server/publications";
import "/imports/api/user/server/methods";

import {initSQL, deinitSQL} from "../imports/api/search/sql";
import {initSearchEventCollection} from "../imports/api/search/collections";

Meteor.startup(async () => {
    await initSQL()
    initSearchEventCollection()
});

process.on('exit', async (code) => {
    console.log(`Node.js is about to exit with code: ${code}`);
    await deinitSQL()
});
