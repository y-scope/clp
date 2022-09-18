// C libraries
#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <sys/stat.h>

// Boost libraries
#include <boost/filesystem.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/compressor_frontend/LogParser.hpp"
#include "../src/compressor_frontend/SchemaParser.hpp"
#include "../src/FileReader.hpp"
#include "../src/streaming_archive/writer/Archive.hpp"
#include "../src/GlobalMetadataDBConfig.hpp"
#include "../src/GlobalSQLiteMetadataDB.hpp"
#include "../src/GlobalMySQLMetadataDB.hpp"
#include "../src/clp/utils.hpp"
#include "../src/clp/FileCompressor.hpp"
#include "../src/ErrorCode.hpp"
#include "../src/Stopwatch.hpp"
#include "../src/clp/clp_main.hpp"

using compressor_frontend::DelimiterStringAST;
using compressor_frontend::LALR1Parser;
using compressor_frontend::Lexer;
using compressor_frontend::LogParser;
using compressor_frontend::ParserAST;
using compressor_frontend::SchemaFileAST;
using compressor_frontend::SchemaParser;
using compressor_frontend::SchemaVarAST;
using compressor_frontend::Token;

std::unique_ptr<SchemaFileAST> generate_schema_ast(std::string schema_file) {
    SchemaParser schema_parser;
    FileReader schema_file_reader;
    schema_file_reader.open(schema_file);
    REQUIRE(schema_file_reader.is_open());
    std::unique_ptr<SchemaFileAST> schema_ast = schema_parser.generate_schema_ast(schema_file_reader);
    REQUIRE(schema_ast.get() != nullptr);
    return schema_ast;
}

std::unique_ptr<LogParser> generate_log_parser(std::string schema_file) {
    std::unique_ptr<SchemaFileAST> schema_ast = generate_schema_ast(schema_file);
    std::unique_ptr<LogParser> log_parser = std::make_unique<LogParser>(schema_file);
    REQUIRE(log_parser.get() != nullptr);
    return log_parser;
}

void parse(std::string log_file, std::string schema_file) {
    std::unique_ptr<LogParser> log_parser = generate_log_parser(schema_file);
    FileReader log_file_reader;
    log_file_reader.open(log_file);
    log_parser->parse(log_file_reader);
}

std::string compress(std::string output_dir, std::string file_to_compress, std::string schema_file, bool old = false) {
    std::vector<std::string> arguments;
    if(old) {
        arguments = {"main.cpp", "c", output_dir, file_to_compress};        
    } else {
        arguments = {"main.cpp", "c", output_dir, file_to_compress, "--schema-path", schema_file};
    }
    std::vector<char*> argv;
    for (const auto& arg : arguments)
        argv.push_back((char*)arg.data());
    argv.push_back(nullptr);
    std::string archive_path;
    clp_main(argv.size() - 1, (const char**) argv.data(), &archive_path);
    return archive_path;
}

std::string decompress(std::string archive_dir, std::string output_dir) {
    std::vector<std::string> arguments = {"main.cpp", "x", archive_dir, output_dir};
    std::vector<char*> argv;
    for (const auto& arg : arguments)
        argv.push_back((char*)arg.data());
    argv.push_back(nullptr);
    std::string archive_path;
    clp_main(argv.size() - 1, (const char**) argv.data(), &archive_path);
    return archive_path;
}

TEST_CASE("Test error for missing schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/missing_schema.txt";
    std::string file_name = boost::filesystem::weakly_canonical(file_path).string();
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), "File not found: " + file_name + "\n");
    std::cout <<"File not found: " + file_name + "\n";
}

TEST_CASE("Test error for empty schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/empty_schema.txt";
    std::string file_name = boost::filesystem::canonical(file_path).string();
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), file_name +":1:1: error: empty file\n"
                                                                  +"          \n"
                                                                  +"^\n");
    std::cout << file_name +":1:1: error: empty file\n"
                 +"          \n"
                 +"^\n";
}

TEST_CASE("Test error for colon missing schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/colon_missing_schema.txt";
    std::string file_name = boost::filesystem::canonical(file_path).string(); 
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), file_name +":3:4: error: expected ':','AlphaNumeric' before ' ' token\n"
                                                                  +"          int [0-9]+\n"
                                                                  +"             ^\n");
}

TEST_CASE("Test error printing with multi-character tokens in schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/schema_with_multicharacter_token_error.txt";
    std::string file_name = boost::filesystem::canonical(file_path).string();
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), file_name +":2:11: error: expected ':' before ' ' token\n"
                                                                  +"          delimiters : \\r\\n\n"
                                                                  +"                    ^\n");
}

TEST_CASE("Test creating schema parser", "[LALR1Parser][SchemaParser]") {
    generate_schema_ast("../tests/test_schema_files/easy_schema.txt");
}

TEST_CASE("Test creating log parser with delimiters", "[LALR1Parser][LogParser]") {
    generate_log_parser("../tests/test_schema_files/schema_with_delimiters.txt");
}

TEST_CASE("Test creating log parser without delimiters", "[LALR1Parser][LogParser]") {
    generate_log_parser("../tests/test_schema_files/schema_without_delimiters.txt");
}

TEST_CASE("Test error for missing log file", "[LALR1Parser][LogParser]") {
    std::string file_name = "../tests/test_log_files/missing_log.txt";
    std::string file_path = boost::filesystem::weakly_canonical(file_name).string();
    compress("../tests/test_archives", file_name, "../tests/test_schema_files/schema_without_delimiters.txt");
    //REQUIRE_THROWS_WITH(compress("../tests/test_archives", file_name, "../tests/test_schema_files/schema_without_delimiters.txt"), 
    //                    "File not found: " + file_path + "\n");
}

TEST_CASE("Test parsing", "[LALR1Parser][LogParser]") {
    compress("../tests/test_archives", "../tests/test_log_files/log.txt", "../tests/test_schema_files/schema_without_delimiters.txt");
}

TEST_CASE("Test compression", "[Compression][Benchmark]") {
    Stopwatch compression_watch("compression_watch");
    std::string compress_dir_path = "../tests/test_archives/openstack_logs_new/";
    std::string input_path = "../tests/test_log_files/openstack_logs/test";
    
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    compression_watch.start();
    std::string archive_path = compress(compress_dir_path, input_path, schema_file);
    std::cout << "Compressed to: " << archive_path << std::endl;
    compression_watch.stop();
    compression_watch.print();
}

TEST_CASE("Test compression old", "[Compression][Benchmark]") {
    Stopwatch compression_watch("compression_watch");
    std::string compress_dir_path = "../tests/test_archives/openstack_logs_old/";
    std::string input_path = "../tests/test_log_files/openstack_logs/test";

    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    compression_watch.start();
    std::string archive_path = compress(compress_dir_path, input_path, schema_file, true);
    std::cout << "Compressed to: " << archive_path << std::endl;
    compression_watch.stop();
    compression_watch.print();
}


TEST_CASE("Test compression without timestamps", "[Compression][Benchmark]") {
    std::string compress_dir_path = "../tests/test_archives/openstack_logs_new/";
    std::string input_path = "../tests/test_log_files/openstack_logs/test_no_timestamps";
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    std::string archive_path = compress(compress_dir_path, input_path, schema_file);
    std::cout << "Compressed to: " << archive_path << std::endl;
}

TEST_CASE("Test decompression", "[Compression][Benchmark]") {
    std::string compress_dir_path = "../tests/test_archives/openstack_logs_new/";
    std::string input_path = "../tests/test_log_files/openstack_logs/test";
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    decompress(compress_dir_path, decompress_dir_path);
}

TEST_CASE("Test compression real", "[Compression][Benchmark]") {
    Stopwatch compression_watch("compression_watch");



    std::string input_path = "../tests/test_log_files/openstack_logs/n-cpu.log.2016-05-08-072252";
    //std::string input_path = "../../../logs";
    //std::string input_path = "../../../logs/yarn--resourcemanager-8d30a2c150ca.log.2018-07-13ir"; //breaks at line 55 if its timestamp format isn't in the schema 
    
    
    std::string compress_dir_path = "../tests/test_archives/openstack_logs_new/";
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    compression_watch.start();
    std::string archive_path = compress(compress_dir_path, input_path, schema_file);
    std::cout << "Compressed to: " << archive_path << std::endl;
    compression_watch.stop();
    compression_watch.print();
}

TEST_CASE("Test compression real old", "[Compression][Benchmark]") {
    Stopwatch compression_watch("compression_watch");
    std::string compress_dir_path = "../tests/test_archives/openstack_logs_old/";
    std::string input_path = "../tests/test_log_files/openstack_logs/n-cpu.log.2016-05-08-072252";
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    compression_watch.start();
    std::string archive_path = compress(compress_dir_path, input_path, schema_file, true);
    std::cout << "Compressed to: " << archive_path << std::endl;
    compression_watch.stop();
    compression_watch.print();
}
 
TEST_CASE("Test compression directory", "[Compression][Benchmark]") {
    Stopwatch compression_watch("compression_watch");
    std::string compress_dir_path = "../tests/test_archives/openstack_logs_new/";
    std::string input_path = "../tests/test_log_files/openstack_logs";
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    compression_watch.start();
    std::string archive_path = compress(compress_dir_path, input_path, schema_file);
    std::cout << "Compressed to: " << archive_path << std::endl;
    compression_watch.stop();
    compression_watch.print();
}

TEST_CASE("Test compression no timestamp", "[Compression][Benchmark]") {
    Stopwatch compression_watch("compression_watch");



    std::string input_path = "../tests/test_log_files/openstack_logs/test_no_timestamp";
    //std::string input_path = "../../../logs";
    //std::string input_path = "../../../logs/yarn--resourcemanager-8d30a2c150ca.log.2018-07-13ir"; //breaks at line 55 if its timestamp format isn't in the schema 


    std::string compress_dir_path = "../tests/test_archives/openstack_logs_new/";
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    compression_watch.start();
    std::string archive_path = compress(compress_dir_path, input_path, schema_file);
    std::cout << "Compressed to: " << archive_path << std::endl;
    compression_watch.stop();
    compression_watch.print();
}

TEST_CASE("Test compression hadoop", "[Compression][Benchmark]") {
    Stopwatch compression_watch("compression_watch");
    std::string compress_dir_path = "../../../archive/";
    std::string input_path = "../../../logs";
    std::string schema_file = "../tests/test_schema_files/real_schema.txt";
    std::string decompress_dir_path = "../tests/decompressed/";
    compression_watch.start();
    std::string archive_path = compress(compress_dir_path, input_path, schema_file);
    std::cout << "Compressed to: " << archive_path << std::endl;
    compression_watch.stop();
    compression_watch.print();
}

TEST_CASE("Test decompression hadoop", "[Compression][Benchmark]") {
    std::string compress_dir_path = "../../../archive/";
    std::string decompress_dir_path = "../../../decompressed/";
    decompress(compress_dir_path, decompress_dir_path);
}

TEST_CASE("Test reverse lexer", "[Search]") {
    FileReader schema_reader;
    ErrorCode error_code = schema_reader.try_open("../tests/test_schema_files/search_schema.txt");
    SchemaParser sp;
    std::unique_ptr<SchemaFileAST> schema_ast = sp.generate_schema_ast(schema_reader);
    Lexer reverse_lexer;
    reverse_lexer.symbol_id[compressor_frontend::cTokenEnd] = reverse_lexer.symbol_id.size();
    reverse_lexer.symbol_id[compressor_frontend::cTokenUncaughtString] = reverse_lexer.symbol_id.size();
    reverse_lexer.id_symbol[(int)compressor_frontend::SymbolID::TokenEndID] = compressor_frontend::cTokenEnd;
    reverse_lexer.id_symbol[(int)compressor_frontend::SymbolID::TokenUncaughtStringID] = compressor_frontend::cTokenUncaughtString;
    auto delimiters_ptr = dynamic_cast<DelimiterStringAST*>(schema_ast->delimiters.get());
    if(delimiters_ptr != nullptr) {
        reverse_lexer.add_delimiters(delimiters_ptr->delimiters);
    }
    for (std::unique_ptr<ParserAST> const& parser_ast : schema_ast->schema_vars) {
        auto rule = dynamic_cast<SchemaVarAST*>(parser_ast.get());
        if( reverse_lexer.symbol_id.find(rule->name) ==  reverse_lexer.symbol_id.end()) {
            reverse_lexer.symbol_id[rule->name] = reverse_lexer.symbol_id.size();
            reverse_lexer.id_symbol[reverse_lexer.symbol_id[rule->name]] = rule->name;

        }
        reverse_lexer.add_rule(reverse_lexer.symbol_id[rule->name], std::move(rule->regex_ptr));
    }
    reverse_lexer.generate_reverse();
    schema_reader.close();
    // use it
    FileReader reader;
    reader.open("../tests/test_search_queries/easy.txt");
    reverse_lexer.reset(reader);
    Token token = reverse_lexer.scan();
    while (token.type_ids->at(0) != (int)compressor_frontend::SymbolID::TokenEndID) {
        std::cout << "token:" << token.get_string() << std::endl;
        std::cout << "token.type_ids->back():" << reverse_lexer.id_symbol[token.type_ids->back()] << std::endl;
        token = reverse_lexer.scan();
    }
}