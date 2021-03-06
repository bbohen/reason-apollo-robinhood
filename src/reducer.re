open Types;

open Utils;

open Actions;

type state = {
  darkTheme: bool,
  quotes: array(quote),
  symbols: array(string),
  symbolInput: string,
};

let httpLinkOptions: ApolloClient.linkOptions = {
  "uri": "http://localhost:8080/graphql",
};

let apolloClientOptions: ApolloClient.clientOptions = {
  "cache": ApolloClient.inMemoryCache(),
  "link": ApolloClient.httpLink(httpLinkOptions),
};

let client = ApolloClient.apolloClient(apolloClientOptions);

let appReducer = (action, state) =>
  switch (action) {
  | LoadInitialQuotesSuccess(data) =>
    let quotes = data##quotes;
    ReasonReact.Update({...state, quotes});
  | AddSymbol =>
    let capitalizedSymbol = String.uppercase(state.symbolInput);
    let symbols = Array.append(state.symbols, [|capitalizedSymbol|]);
    ReasonReact.UpdateWithSideEffects(
      state,
      (
        self => {
          let variables = {"symbols": symbols};
          let options = {"query": Queries.quotes, "variables": variables};
          client##query(options)
          |> Js.Promise.then_(response => {
               let responseData = response##data;
               let leng = Array.length(responseData##quotes);
               let quotes: array(possibleQuote) = responseData##quotes;
               let newQuote = quotes[leng - 1];
               let symbolsList = Array.to_list(state.symbols);
               let isDupe =
                 List.exists(
                   element => element == capitalizedSymbol,
                   symbolsList,
                 );
               let nullableQuote =
                 isDupe ? None : Js.Nullable.toOption(newQuote);
               switch (nullableQuote) {
               | None => Js.log("No matching symbol, should probably add messaging for this!")
               | Some(quote) =>
                 let quoteToAdd = {
                   "ask_price": quote##ask_price,
                   "description": quote##description,
                   "high_52_weeks": quote##high_52_weeks,
                   "historical_data": [||],
                   "low_52_weeks": quote##low_52_weeks,
                   "last_trade_price": quote##last_trade_price,
                   "simple_name": quote##simple_name,
                   "symbol": quote##symbol,
                   "trade_price_delta": quote##trade_price_delta,
                   "trade_price_delta_percentage": quote##trade_price_delta_percentage,
                   "hours_since_update": quote##hours_since_update,
                 };
                 self.send(AddSymbolSuccess(quoteToAdd));
                 ();
               };
               Js.Promise.resolve();
             })
          |> ignore;
          ();
        }
      ),
    );
  | AddSymbolSuccess(quote) =>
    let quotes = Array.append(state.quotes, [|quote|]);
    let capitalizedSymbol = String.uppercase(state.symbolInput);
    let symbols = Array.append(state.symbols, [|capitalizedSymbol|]);
    saveLocally(symbols);
    ReasonReact.Update({...state, quotes, symbols, symbolInput: ""});
  | LoadHistoricalDataForQuote(payload) =>
    let variables = {"symbol": payload##symbol};
    let options = {"query": Queries.historicalData, "variables": variables};
    ReasonReact.UpdateWithSideEffects(
      state,
      (
        self => {
          client##query(options)
          |> Js.Promise.then_(response => {
               let responseData = response##data;
               let history = responseData##history;
               let adjustedHistory =
                 Array.map(item => item##close_price, history);
               let relatedQuote = state.quotes[payload##index];
               let adjustedQuote = {
                 "ask_price": relatedQuote##ask_price,
                 "description": relatedQuote##description,
                 "high_52_weeks": relatedQuote##high_52_weeks,
                 "historical_data": adjustedHistory,
                 "low_52_weeks": relatedQuote##low_52_weeks,
                 "last_trade_price": relatedQuote##last_trade_price,
                 "simple_name": relatedQuote##simple_name,
                 "symbol": relatedQuote##symbol,
                 "trade_price_delta": relatedQuote##trade_price_delta,
                 "trade_price_delta_percentage": relatedQuote##trade_price_delta_percentage,
                 "hours_since_update": relatedQuote##hours_since_update,
               };
               self.send(LoadHistoricalDataForQuoteSuccess(adjustedQuote, payload##index));
               Js.Promise.resolve();
             })
          |> ignore;
          ();
        }
      ),
    );
  | LoadHistoricalDataForQuoteSuccess(quote, index) =>
    let updatedQuotes = Array.copy(state.quotes);
    updatedQuotes[index] = quote;
    ReasonReact.Update({...state, quotes: updatedQuotes});
  | RemoveSymbol(clickedSymbol) =>
    let symbolIter = symbol => symbol != clickedSymbol;
    let newSymbolArray = Js.Array.filter(symbolIter, state.symbols);
    if (Array.length(newSymbolArray) > 0) {
      ReasonReact.UpdateWithSideEffects(
        state,
        (
          self => {
            let variables = {"symbols": newSymbolArray};
            let options = {"query": Queries.quotes, "variables": variables};
            client##query(options)
            |> Js.Promise.then_(response => {
                 let responseData = response##data;
                 self.send(RemoveSymbolSuccess(newSymbolArray, responseData));
                 Js.Promise.resolve();
               })
            |> ignore;
            ();
          }
        ),
      );
    } else {
      ReasonReact.Update({...state, quotes: [||], symbols: [||]});
    };
  | RemoveSymbolSuccess(symbols, data) =>
    let quotes = data##quotes;
    saveLocally(symbols);
    ReasonReact.Update({...state, quotes, symbols});
  | ToggleTheme =>
    ReasonReact.Update({...state, darkTheme: ! state.darkTheme})
  | UpdateSymbolInput(value) =>
    ReasonReact.Update({...state, symbolInput: value})
  };
