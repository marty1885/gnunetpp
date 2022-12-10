# GNUnet++ - A C++ wrapper for GNUnet

## Why GNUnet++?

IMO GNUnet is a cool project which shows what decentralized systems could look like. However the [time to first hello world][ttfhw] is way too long, API is not well documented and everything is written GNU style C. Which are all factors against it's adoption. GNUnet++ wraps all the C-ness into a (more or less) modern C++ style highlevel API. Provides proper lifetime management and an easier learning path. Hopefully leading to a better development experience.

**IMPORTAN**: This is not a part of the GNUnet project. The [author](https://github.com/marty1885) created it because he wants to use GNUnet in C++.

**ALSO IMPORTANT**: Don't expect the code base be clean. GNUnet is a _very_ C project. Lifetime managment is all over the place. I did my best to create a sane API out of it. Nor it is production ready. The API might go through major changes in the future. Nor ABI stability is guaranteed.

[ttfhw]: https://www.moesif.com/blog/technical/api-product-management/What-is-TTFHW/ 

## Show me some code!

The DHT class provides easy access to GNUnet's DHT. It's API is designed to look like [OpenDHT][opendht]'s for more familiarity to potential users. Note that need GNUnet running on your system before running the samples.

```cpp
// Max 32 simultaneous operations
auto dht = std::make_shared<DHT>(cfg, 32);

...
dht->put("Answer", "42");
dht->get("some_super_secret", [](const std::string_view data) {
   std::cout << "super secret info: " << data << std::endl;
   return true; // keep on searching after the current one.
   // return false to cancel futher lookups.
}, std::chrono::seconds(30)); // Max searching for 30 seconds
```

The cryptographic library is also easily accessable

```cpp
auto hash = crypto::hash("foobar");
auto hmac = crypto::hmac("hmac_key", "zap");
std::cout << crypto::to_string(hmac) << "\n";
```

Search and download files from the GNUnet File Sharing service.

```cpp
FS::search({"jpg"}, [](const std::string_view uri, const std::string_view name) {
   std::cout << "Found " << filename << " at " << std::endl << std::endl;
   return true;
}, std::chrono::seconds(30));

FS::downlad("gnunet://fs/...", "foobar.jpg", [](DownloadStatus status) {
    if(status == DownloadStatus::Completed)
      // yay! we're done
});
```

Wich generates the following output

```
Found IMG_20220528_134243_161.jpg at gnunet://fs/chk/N59XZ8KPMQ0975JBTPV9AGEAD5T7V694QYWNK21683Q74TTRMQB2CHW4AZVTM3A5NFC57K0N6PD5EGCGMJABTZ6HKMV9ZC1T52FTVSG.2RJ19QYFPBJ2TBMZSNECXP9KHDTX90B6ZCTBSJYQKPK016156HNCPE5RJMNEM3A1NTRHMVWK8GCJ1MVG4S25F8A4TW1S70PCDMSG94R.2778934

Found 1651945314526m.jpg at gnunet://fs/chk/R4MCF6MTQDR4VKKKMD1H8PD9THRKGV1ER0BXP57BKVT2580KW7S25HFSW7MK0BM1JBPBEHG6P0SHDHDERX7MTPFA5YE68E7Q43H8Z78.4J604CQR9AESPQ3X894PE2P56X3P21QJWBBQQXH4SR07X4KXX5TBH62BHSDT6HWY70XP5DZB5S5FADDJ7TDYENEX67H4JN6Q1KP725G.91907

Found ball.jpg at gnunet://fs/chk/3ZJRZJRDD6V6R54TG9VAC4G3ZQ4WGZ5ZVSP5BZ12X004CYTGDNTB9P8STZ0P1Y2REB28EA8FZ3JZ4900V5FVEMYAESDWVGATZ37WJAR.46BHSJS8BXTT6KN4NTBS66VAYSDKRFST71439H6RAAKPT294T3ECY6AEQCN726ZQXW039YD7Z0Q17385HMH8RQWT92AR8AQ4B47X60R.241099
...
```

Examples with comments can be found in the [examples](examples) directory.

[opendht]: https://github.com/savoirfairelinux/opendht

## Requirments

First you need the dependencies

* C++17 capable compiler
* A installation of GNUnet 0.19 (or likely the latest version)

(For examples)
* CLI11

## Roadmap

This project aims to create a easy to use wapper for the commonly used part of GNUnet.

- DHT
  - [x] Basic operations (put/get)
  - [ ] Monitor
- File Sharing
  - [x] Download
  - [x] Publish
  - [x] Search
- Crypto
  - [x] Hash
  - [x] HMAC
  - [x] ECDSA/EDDSA
  - [x] to_string
  - [ ] RSA
- CADET
  - [ ] Server
  - [ ] Client
- GNS
  - [ ] Resolve
  - [ ] Register/modify record
  - [ ] Ego
- Identity
  - [x] Create
  - [x] Delte
  - [x] Get Ego
- Scheduler
  - [x] Delayed run
  - [x] Run on exit
  - [x] Run immidately
