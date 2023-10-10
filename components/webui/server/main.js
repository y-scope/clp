import {Meteor} from "meteor/meteor";

import {StatsCollection} from "/imports/api/ingestion/publications";
import "/imports/api/ingestion/server/publications";
import "/imports/api/search/server/constants";
import "/imports/api/search/server/methods";
import "/imports/api/search/server/publications";
import "/imports/api/search/server/query_handler_mediator";
import "/imports/api/user/server/methods";

Meteor.startup(() => {
  if (StatsCollection.find().count() === 0) {
    StatsCollection.insert({
      total_uncompressed_size: 1000,
      total_compressed_size: 10,
      begin_ts: 0,
      end_ts: 0,
      num_files: 20,
      num_messages: 10000,
    });
  }
});
