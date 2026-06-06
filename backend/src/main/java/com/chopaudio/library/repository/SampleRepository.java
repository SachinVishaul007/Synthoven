package com.chopaudio.library.repository;

import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.SampleType;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface SampleRepository extends JpaRepository<Sample, Long> {

    Optional<Sample> findByFilePath(String filePath);

    List<Sample> findByTypeOrderByCreatedAtDesc(SampleType type);

    List<Sample> findByFavoriteTrueOrderByCreatedAtDesc();

    List<Sample> findByLastPlayedAtNotNullOrderByLastPlayedAtDesc(Pageable pageable);

    boolean existsByFilePath(String filePath);

    long countByType(SampleType type);

    @Query("""
            SELECT DISTINCT s FROM Sample s
            LEFT JOIN s.tags t
            WHERE LOWER(s.name) LIKE LOWER(CONCAT('%', :q, '%'))
               OR LOWER(t) LIKE LOWER(CONCAT('%', :q, '%'))
               OR LOWER(COALESCE(s.musicalKey, '')) LIKE LOWER(CONCAT('%', :q, '%'))
            ORDER BY s.favorite DESC, s.playCount DESC
            """)
    List<Sample> search(@Param("q") String query, Pageable pageable);
}
