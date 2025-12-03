#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <vector>

#include <pnq/pnq.h>
#include <pnq/directory.h>
#include <pnq/file.h>

namespace pnq
{
    /// Helper class for reading/modifying the Windows hosts file.
    /// Preserves comments, blank lines, and file structure.
    class HostsFile
    {
    public:
        struct Entry
        {
            std::string ip;
            std::string hostname;
            std::string comment;
        };

        /// Get the system hosts file path.
        static std::string system_path()
        {
            return directory::windows() + "\\System32\\drivers\\etc\\hosts";
        }

        /// Load hosts file from path (defaults to system hosts file).
        bool load(const std::string& path = {})
        {
            m_path = path.empty() ? system_path() : path;
            m_lines.clear();

            std::ifstream file(m_path);
            if (!file.is_open())
            {
                spdlog::warn("Failed to open hosts file: {}", m_path);
                return false;
            }

            std::string line;
            while (std::getline(file, line))
                m_lines.push_back(line);

            return true;
        }

        /// Load hosts file from string content.
        void load_from_string(std::string_view content)
        {
            m_path.clear();
            m_lines.clear();

            std::istringstream iss{std::string{content}};
            std::string line;
            while (std::getline(iss, line))
                m_lines.push_back(line);
        }

        /// Save hosts file (creates timestamped backup first if path was loaded from file).
        bool save()
        {
            if (m_path.empty())
                return false;

            if (!create_backup())
                return false;

            std::ofstream file(m_path);
            if (!file.is_open())
            {
                spdlog::error("Failed to open hosts file for writing: {}", m_path);
                return false;
            }

            for (size_t i = 0; i < m_lines.size(); ++i)
            {
                file << m_lines[i];
                if (i + 1 < m_lines.size())
                    file << '\n';
            }

            if (!m_lines.empty() && !m_lines.back().empty())
                file << '\n';

            return true;
        }

        /// Export current content as string.
        std::string to_string() const
        {
            std::ostringstream oss;
            for (size_t i = 0; i < m_lines.size(); ++i)
            {
                oss << m_lines[i];
                if (i + 1 < m_lines.size())
                    oss << '\n';
            }
            return oss.str();
        }

        /// Find entry by hostname (case-insensitive).
        std::optional<Entry> find(std::string_view hostname) const
        {
            for (const auto& line : m_lines)
            {
                if (auto entry = parse_line(line))
                {
                    if (contains_hostname(*entry, hostname))
                        return Entry{entry->ip, std::string{hostname}, entry->comment};
                }
            }
            return std::nullopt;
        }

        /// Check if hostname exists.
        bool contains(std::string_view hostname) const
        {
            return find(hostname).has_value();
        }

        /// Add or update entry for hostname.
        void set(std::string_view hostname, std::string_view ip, std::string_view comment = {})
        {
            // Try to update existing
            for (auto& line : m_lines)
            {
                if (auto entry = parse_line(line))
                {
                    if (contains_hostname(*entry, hostname))
                    {
                        line = format_entry(ip, hostname, comment);
                        return;
                    }
                }
            }

            // Append new entry
            m_lines.push_back(format_entry(ip, hostname, comment));
        }

        /// Remove entry containing hostname.
        bool remove(std::string_view hostname)
        {
            bool removed = false;
            std::vector<std::string> filtered;

            for (const auto& line : m_lines)
            {
                auto entry = parse_line(line);
                if (entry && contains_hostname(*entry, hostname))
                    removed = true;
                else
                    filtered.push_back(line);
            }

            m_lines = std::move(filtered);
            return removed;
        }

        /// Get all entries (skips comments and blank lines).
        std::vector<Entry> entries() const
        {
            std::vector<Entry> result;
            for (const auto& line : m_lines)
            {
                if (auto parsed = parse_line(line))
                {
                    for (const auto& hostname : parsed->hostnames)
                        result.push_back(Entry{parsed->ip, hostname, parsed->comment});
                }
            }
            return result;
        }

        /// Get the loaded file path.
        const std::string& path() const { return m_path; }

        /// Get raw line count.
        size_t line_count() const { return m_lines.size(); }

    private:
        struct ParsedLine
        {
            std::string ip;
            std::vector<std::string> hostnames;
            std::string comment;
        };

        static std::optional<ParsedLine> parse_line(const std::string& line)
        {
            // Skip empty lines and comment-only lines
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos || line[start] == '#')
                return std::nullopt;

            ParsedLine result;

            // Extract comment
            std::string content = line;
            if (size_t pos = line.find('#'); pos != std::string::npos)
            {
                result.comment = line.substr(pos + 1);
                if (size_t cstart = result.comment.find_first_not_of(" \t"); cstart != std::string::npos)
                    result.comment = result.comment.substr(cstart);
                else
                    result.comment.clear();
                content = line.substr(0, pos);
            }

            // Parse IP and hostnames
            std::istringstream iss(content);
            iss >> result.ip;
            if (result.ip.empty())
                return std::nullopt;

            std::string hostname;
            while (iss >> hostname)
                result.hostnames.push_back(hostname);

            if (result.hostnames.empty())
                return std::nullopt;

            return result;
        }

        static bool contains_hostname(const ParsedLine& entry, std::string_view hostname)
        {
            for (const auto& h : entry.hostnames)
            {
                if (string::equals_nocase(h, hostname))
                    return true;
            }
            return false;
        }

        static std::string format_entry(std::string_view ip, std::string_view hostname, std::string_view comment)
        {
            std::ostringstream oss;
            oss << ip << "\t" << hostname;
            if (!comment.empty())
                oss << " # " << comment;
            return oss.str();
        }

        bool create_backup()
        {
            if (!file::exists(m_path))
                return true;

            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_s(&tm, &time);

            std::ostringstream backup_name;
            backup_name << m_path << ".backup-"
                        << std::put_time(&tm, "%Y%m%d-%H%M%S");

            std::string backup_path = backup_name.str();

            if (file::exists(backup_path))
                return true;

            std::error_code ec;
            std::filesystem::copy_file(m_path, backup_path, ec);
            if (ec)
            {
                spdlog::error("Failed to backup hosts file to {}: {}", backup_path, ec.message());
                return false;
            }

            spdlog::info("Created hosts file backup: {}", backup_path);
            return true;
        }

        std::string m_path;
        std::vector<std::string> m_lines;
    };

} // namespace pnq
