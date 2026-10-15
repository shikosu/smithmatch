# =============================================================================
# EmbedResource.cmake — Transforme un fichier binaire (ex. police .ttf) en
# tableau C++ compilé DANS l'exécutable.
# =============================================================================
# Pourquoi ? Si la police restait un fichier à côté du .exe, il faudrait la
# retrouver à l'exécution (chemin différent sur Windows, dans un bundle macOS…).
# En la mettant directement dans le programme, elle est toujours là.
#
# Ce fichier sert à DEUX choses :
#   1. Inclus normalement (include(...)) : il définit la fonction
#      smith_embed_resource() utilisée dans CMakeLists.txt.
#   2. Exécuté en « mode script » (cmake -P EmbedResource.cmake) : il fait la
#      conversion elle-même, au moment de la compilation.
# =============================================================================

if(CMAKE_SCRIPT_MODE_FILE)
    # ---------------------------------------------------------------------
    # Mode script : variables attendues INPUT, OUTPUT, SYMBOL, NAMESPACE.
    # ---------------------------------------------------------------------
    file(READ "${INPUT}" content HEX)           # tout le fichier, en hexadécimal
    string(LENGTH "${content}" hexLength)
    math(EXPR byteCount "${hexLength} / 2")

    # Un retour à la ligne tous les 16 octets (32 caractères hexa) :
    # les compilateurs n'aiment pas les lignes de plusieurs mégaoctets.
    string(REGEX REPLACE "([0-9a-f]{32})" "\\1\n" content "${content}")
    # Chaque paire « ab » devient « 0xab, ».
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," content "${content}")

    get_filename_component(inputName "${INPUT}" NAME)
    file(WRITE "${OUTPUT}"
        "// Fichier GÉNÉRÉ automatiquement par cmake/EmbedResource.cmake — ne pas modifier.\n"
        "// Source : ${inputName} (${byteCount} octets)\n"
        "#include <cstddef>\n\n"
        "namespace ${NAMESPACE} {\n"
        "extern const unsigned char ${SYMBOL}[] = {\n${content}\n};\n"
        "extern const std::size_t ${SYMBOL}Size = ${byteCount};\n"
        "} // namespace ${NAMESPACE}\n")
    return()
endif()

# -----------------------------------------------------------------------------
# smith_embed_resource(<target> <fichier_entrée> <nom_symbole>)
# Ajoute au <target> un .cpp généré qui contient :
#   const unsigned char <nom_symbole>[];     // les octets du fichier
#   const std::size_t   <nom_symbole>Size;   // leur nombre
# dans le namespace smith::assets. Le .cpp est régénéré si le fichier change.
# -----------------------------------------------------------------------------
set(SMITH_EMBED_SCRIPT "${CMAKE_CURRENT_LIST_FILE}")

function(smith_embed_resource target input symbol)
    set(output "${CMAKE_BINARY_DIR}/generated/${symbol}.cpp")
    add_custom_command(
        OUTPUT  "${output}"
        COMMAND ${CMAKE_COMMAND}
                -DINPUT=${input}
                -DOUTPUT=${output}
                -DSYMBOL=${symbol}
                -DNAMESPACE=smith::assets
                -P "${SMITH_EMBED_SCRIPT}"
        DEPENDS "${input}" "${SMITH_EMBED_SCRIPT}"
        COMMENT "Embedding ${input}"
        VERBATIM)
    target_sources(${target} PRIVATE "${output}")
endfunction()
